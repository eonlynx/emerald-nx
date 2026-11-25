/*
 * Emerald NX two-stage loader.
 *
 * hbloader maps NROs with virtmemFindCodeMemory, which can return an address
 * above 4 GiB. Emerald keeps 32-bit addresses in its data, so this loader
 * reserves a low range (128 MiB..2 GiB), maps emerald-game.nro there with
 * svcMapProcessCodeMemory, provides a low heap/stack through the homebrew ABI,
 * and jumps to the game entrypoint.
 *
 * Based on nx-hbloader's loadNro (MIT) and toniisound/smbr-nx's so_util.
 */
#include <switch.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "jump.h"

#define APP_DIR "sdmc:/switch/emerald-nx"
#define GAME_NRO APP_DIR "/emerald-game.nro"
#define LOADER_LOG APP_DIR "/loader.log"

#define LOW_START 0x08000000ULL
#define LOW_END 0x80000000ULL
#define PAGE 0x1000ULL
#define HEAP_SIZE (256ULL * 1024ULL * 1024ULL)
#define STACK_SIZE (8ULL * 1024ULL * 1024ULL)

static FILE *g_log;

static void log_line(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    if (g_log) {
        va_start(args, fmt);
        vfprintf(g_log, fmt, args);
        va_end(args);
        fputc('\n', g_log);
        fflush(g_log);
    }
    consoleUpdate(NULL);
}

static void fail(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    if (g_log) {
        va_start(args, fmt);
        vfprintf(g_log, fmt, args);
        va_end(args);
        fputc('\n', g_log);
        fflush(g_log);
    }
    consoleUpdate(NULL);
    svcSleepThread(5'000'000'000ULL);
    svcExitProcess();
}

static uint64_t align_up(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static void *find_low_gap(size_t size) {
    uint64_t addr = LOW_START;
    while (addr < LOW_END) {
        MemoryInfo info;
        u32 page_info = 0;
        if (R_FAILED(svcQueryMemory(&info, &page_info, addr))) {
            return NULL;
        }
        const uint64_t region_end = info.addr + info.size;
        if (region_end <= addr) {
            return NULL;
        }
        if (info.type == MemType_Unmapped) {
            uint64_t start = info.addr < LOW_START ? LOW_START : info.addr;
            start = align_up(start, PAGE);
            if (start + size <= region_end) {
                return (void *)(uintptr_t)start;
            }
        }
        addr = region_end;
    }
    return NULL;
}

static void *map_low(Handle process, void *source, size_t size, u32 permission,
                     bool apply_permission, const char *what) {
    armDCacheFlush(source, size);

    if ((size & (PAGE - 1)) != 0 || size == 0) {
        fail("loader: %s size is not page aligned", what);
    }

    virtmemLock();
    void *destination = find_low_gap(size);
    VirtmemReservation *reservation = destination ? virtmemAddReservation(destination, size) : NULL;
    virtmemUnlock();
    if (!destination || !reservation) {
        fail("loader: no low address gap for %s (%" PRIu64 " bytes)", what, (uint64_t)size);
    }

    Result rc = svcMapProcessCodeMemory(process, (u64)(uintptr_t)destination,
                                        (u64)(uintptr_t)source, size);
    if (R_FAILED(rc)) {
        fail("loader: svcMapProcessCodeMemory(%s) failed: 0x%08x", what, rc);
    }
    if (apply_permission) {
        rc = svcSetProcessMemoryPermission(process, (u64)(uintptr_t)destination, size, permission);
        if (R_FAILED(rc)) {
            fail("loader: svcSetProcessMemoryPermission(%s) failed: 0x%08x", what, rc);
        }
    }
    armICacheInvalidate(destination, size);
    log_line("loader: %s at %p (%" PRIu64 " bytes, perm %s)", what, destination,
             (uint64_t)size, apply_permission ? "applied" : "per segment");
    return destination;
}

static void *load_nro_file(const char *path, size_t *out_total, NroHeader *out_header) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fail("loader: cannot open %s", path);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        fail("loader: cannot seek %s", path);
    }
    const long file_size = ftell(file);
    if (file_size < (long)(sizeof(NroStart) + sizeof(NroHeader))) {
        fclose(file);
        fail("loader: %s is too small", path);
    }
    rewind(file);

    size_t capacity = (size_t)align_up((uint64_t)file_size, PAGE);
    void *buffer = aligned_alloc(PAGE, capacity);
    if (!buffer) {
        fclose(file);
        fail("loader: out of memory for %s", path);
    }
    memset(buffer, 0, capacity);
    if (fread(buffer, 1, (size_t)file_size, file) != (size_t)file_size) {
        fclose(file);
        fail("loader: short read on %s", path);
    }
    fclose(file);

    const NroHeader *header = (const NroHeader *)((const char *)buffer + sizeof(NroStart));
    if (header->magic != NROHEADER_MAGIC) {
        fail("loader: %s is not an NRO (magic 0x%08x)", path, header->magic);
    }
    const size_t total = (size_t)align_up((uint64_t)header->size + header->bss_size, PAGE);
    if (total > capacity) {
        void *larger = aligned_alloc(PAGE, total);
        if (!larger) {
            fail("loader: out of memory for %s bss", path);
        }
        memset(larger, 0, total);
        memcpy(larger, buffer, (size_t)file_size);
        free(buffer);
        buffer = larger;
        capacity = total;
    }
    (void)capacity;
    *out_total = total;
    *out_header = *header;
    log_line("loader: %s: file %ld, image %" PRIu64 ", bss %u, total %" PRIu64,
             path, file_size, (uint64_t)header->size, header->bss_size, (uint64_t)total);
    return buffer;
}

void switch_after_game(void) {
    consoleInit(NULL);
    log_line("loader: game returned; ending the process to keep hbloader safe");
    fflush(g_log);
    consoleUpdate(NULL);
    svcExitProcess();
}

static void set_nro_permissions(Handle process, void *base, const NroHeader *header) {
    const uint64_t text = (uint64_t)(uintptr_t)base + header->segments[0].file_off;
    const uint64_t rodata = (uint64_t)(uintptr_t)base + header->segments[1].file_off;
    const uint64_t data = (uint64_t)(uintptr_t)base + header->segments[2].file_off;
    const uint64_t text_size = align_up(header->segments[0].size, PAGE);
    const uint64_t rodata_size = align_up(header->segments[1].size, PAGE);
    const uint64_t data_size = align_up((uint64_t)header->segments[2].size + header->bss_size, PAGE);

    Result rc = svcSetProcessMemoryPermission(process, text, text_size, Perm_R | Perm_X);
    if (R_FAILED(rc)) fail("loader: text permission failed: 0x%08x", rc);
    rc = svcSetProcessMemoryPermission(process, rodata, rodata_size, Perm_R);
    if (R_FAILED(rc)) fail("loader: rodata permission failed: 0x%08x", rc);
    rc = svcSetProcessMemoryPermission(process, data, data_size, Perm_Rw);
    if (R_FAILED(rc)) fail("loader: data permission failed: 0x%08x", rc);
    armICacheInvalidate((void *)(uintptr_t)text, text_size);

    log_line("loader: segments text %" PRIu64 " rodata %" PRIu64 " data %" PRIu64,
             text_size, rodata_size, data_size);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    mkdir("sdmc:/switch", 0777);
    mkdir(APP_DIR, 0777);
    fsdevMountSdmc();
    g_log = fopen(LOADER_LOG, "w");
    if (g_log) {
        setvbuf(g_log, NULL, _IOLBF, 0);
    }

    log_line("Emerald NX loader");
    log_line("loader: applet type %d, own process handle 0x%x, main thread 0x%x",
             appletGetAppletType(), envGetOwnProcessHandle(), envGetMainThreadHandle());

    const Handle process = envGetOwnProcessHandle();
    if (process == INVALID_HANDLE) {
        fail("loader: no own-process handle; launch as a title takeover from hbmenu");
    }

    size_t image_size = 0;
    NroHeader header;
    void *image = load_nro_file(GAME_NRO, &image_size, &header);
    /* Do not make the whole image RW first: the kernel forbids a later
     * W -> X transition on code memory.  Set text RX/rodata R/data RW
     * directly, as hbloader does. */
    void *game_base = map_low(process, image, image_size, 0, false, "game image");
    set_nro_permissions(process, game_base, &header);

    void *heap_source = aligned_alloc(PAGE, (size_t)HEAP_SIZE);
    if (!heap_source) {
        fail("loader: out of memory for game heap");
    }
    memset(heap_source, 0, (size_t)HEAP_SIZE);
    void *heap_base = map_low(process, heap_source, (size_t)HEAP_SIZE, Perm_Rw, true, "game heap");

    void *stack_top = (char *)heap_base + STACK_SIZE;
    void *game_heap = stack_top;
    const uint64_t game_heap_size = HEAP_SIZE - STACK_SIZE;

    log_line("loader: game base %p, stack top %p, heap %p..%p", game_base, stack_top,
             game_heap, (char *)game_heap + game_heap_size);

    if ((uint64_t)(uintptr_t)game_base >= 0x100000000ULL ||
        (uint64_t)(uintptr_t)stack_top >= 0x100000000ULL ||
        (uint64_t)(uintptr_t)game_heap + game_heap_size > 0x100000000ULL) {
        fail("loader: low mapping escaped the 32-bit range");
    }

    static AccountUid user_id;
    static char loader_info[] = "Emerald NX loader";
    static char next_path[512] = "";
    static char next_argv[512] = "";
    static char game_argv[8] = "";
    ConfigEntry entries[16];
    size_t index = 0;

    const AppletType current_applet = appletGetAppletType();
    const u64 applet_type = current_applet == AppletType_LibraryApplet
                                ? AppletType_LibraryApplet
                                : AppletType_SystemApplication;
    const u64 applet_flags = applet_type == AppletType_SystemApplication
                                 ? EnvAppletFlags_ApplicationOverride
                                 : 0;

    entries[index++] = (ConfigEntry){EntryType_MainThreadHandle, 0, {envGetMainThreadHandle(), 0}};
    entries[index++] = (ConfigEntry){EntryType_ProcessHandle, 0, {process, 0}};
    entries[index++] = (ConfigEntry){EntryType_OverrideHeap, EntryFlag_IsMandatory, {(u64)(uintptr_t)game_heap, game_heap_size}};
    entries[index++] = (ConfigEntry){EntryType_Argv, 0, {0, (u64)(uintptr_t)game_argv}};
    entries[index++] = (ConfigEntry){EntryType_NextLoadPath, 0, {(u64)(uintptr_t)next_path, (u64)(uintptr_t)next_argv}};
    entries[index++] = (ConfigEntry){EntryType_AppletType, 0, {applet_type, applet_flags}};
    entries[index++] = (ConfigEntry){EntryType_SyscallAvailableHint, 0, {UINT64_MAX, UINT64_MAX}};
    entries[index++] = (ConfigEntry){EntryType_SyscallAvailableHint2, 0, {UINT64_MAX, 0}};
    entries[index++] = (ConfigEntry){EntryType_LastLoadResult, 0, {0, 0}};
    entries[index++] = (ConfigEntry){EntryType_RandomSeed, 0, {randomGet64(), randomGet64()}};
    entries[index++] = (ConfigEntry){EntryType_UserIdStorage, 0, {(u64)(uintptr_t)&user_id, 0}};
    entries[index++] = (ConfigEntry){EntryType_HosVersion, 0, {hosversionGet(), 0x41544d4f53504852ULL}};
    entries[index++] = (ConfigEntry){EntryType_EndOfList, 0, {(u64)(uintptr_t)loader_info, sizeof(loader_info)}};

    log_line("loader: jumping to game (applet %" PRIu64 ", flags %" PRIu64 ", heap %" PRIu64 ")",
             applet_type, applet_flags, game_heap_size);
    fflush(g_log);
    consoleUpdate(NULL);
    /* The game NRO has its own libnx console state. */
    consoleExit(NULL);

    switch_jump_to_nro(entries, game_base, stack_top, process);
    fail("loader: game returned unexpectedly");
    return 0;
}
