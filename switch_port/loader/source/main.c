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
static u8 sSavedTls[0x100];

static void log_line(const char *fmt, ...) {
    va_list args;
    if (g_log) {
        va_start(args, fmt);
        vfprintf(g_log, fmt, args);
        va_end(args);
        fputc('\n', g_log);
        fflush(g_log);
    }
}

static void fail(const char *fmt, ...) {
    va_list args;
    if (g_log) {
        va_start(args, fmt);
        vfprintf(g_log, fmt, args);
        va_end(args);
        fputc('\n', g_log);
        fflush(g_log);
    }
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
    if ((size & (PAGE - 1)) != 0 || size == 0) {
        fail("loader: %s size is not page aligned", what);
    }

    log_line("loader: mapping %s (%" PRIu64 " bytes)...", what, (uint64_t)size);
    armDCacheFlush(source, size);

    virtmemLock();
    void *destination = find_low_gap(size);
    VirtmemReservation *reservation = destination ? virtmemAddReservation(destination, size) : NULL;
    virtmemUnlock();
    if (!destination || !reservation) {
        fail("loader: no low address gap for %s (%" PRIu64 " bytes)", what, (uint64_t)size);
    }
    log_line("loader: %s destination %p", what, destination);

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
    /* Code pages are not readable until set_nro_permissions has given them
     * their final permission, so the game image is invalidated there, after
     * the RX ranges exist.  The heap is data and needs no instruction cache
     * maintenance. */
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
    NroStart start;
    NroHeader header;
    if (file_size < (long)(sizeof(NroStart) + sizeof(NroHeader))) {
        fclose(file);
        fail("loader: %s is too small", path);
    }
    rewind(file);
    if (fread(&start, 1, sizeof(start), file) != sizeof(start) ||
        fread(&header, 1, sizeof(header), file) != sizeof(header)) {
        fclose(file);
        fail("loader: short read on %s header", path);
    }
    (void)start;
    if (header.magic != NROHEADER_MAGIC) {
        fclose(file);
        fail("loader: %s is not an NRO (magic 0x%08x)", path, header.magic);
    }
    if (header.size < sizeof(NroStart) + sizeof(NroHeader) ||
        (size_t)file_size < (size_t)header.size) {
        fclose(file);
        fail("loader: %s is truncated (image %u, file %ld)", path, header.size, file_size);
    }

    /* Only header.size bytes belong to the image; the icon/NACP and any other
     * NRO assets follow it and must not end up in the bss area. */
    const size_t total = (size_t)align_up((uint64_t)header.size + header.bss_size, PAGE);
    void *buffer = aligned_alloc(PAGE, total);
    if (!buffer) {
        fclose(file);
        fail("loader: out of memory for %s", path);
    }
    memset(buffer, 0, total);
    rewind(file);
    if (fread(buffer, 1, (size_t)header.size, file) != (size_t)header.size) {
        free(buffer);
        fclose(file);
        fail("loader: short read on %s", path);
    }
    fclose(file);

    *out_total = total;
    *out_header = header;
    log_line("loader: %s: file %ld, image %u, bss %u, total %" PRIu64,
             path, file_size, header.size, header.bss_size, (uint64_t)total);
    return buffer;
}

void switch_after_game(void) {
    log_line("loader: game returned; ending the process to keep hbloader safe");
    fflush(g_log);
    svcExitProcess();
}

/*
 * NRO segment permissions, following smbr-nx's so_util: the kernel never
 * allows a W -> X transition on code memory, so set every executable page RX
 * first, straight from the freshly mapped state, then every other page RW.
 * A page map (rather than three independent ranges) also covers the padding
 * between segments and a segment that shares a page with the next one.
 */
enum { PAGE_RW = 0, PAGE_RX = 1, PAGE_R = 2 };

static void mark_pages(uint8_t *kind, size_t num_pages, uint64_t offset, uint64_t size,
                       uint8_t value) {
    const uint64_t first = offset / PAGE;
    const uint64_t last = (offset + size + PAGE - 1) / PAGE;
    for (uint64_t pg = first; pg < last && pg < num_pages; ++pg) {
        if (kind[pg] == PAGE_RX) {
            continue; /* an executable page wins a shared boundary */
        }
        kind[pg] = value;
    }
}

static void set_nro_permissions(Handle process, void *base, const NroHeader *header,
                                size_t image_size) {
    const size_t num_pages = image_size / PAGE;
    uint8_t *kind = calloc(num_pages, 1);
    if (!kind) {
        fail("loader: out of memory for the page map");
    }

    mark_pages(kind, num_pages, header->segments[0].file_off, header->segments[0].size, PAGE_RX);
    mark_pages(kind, num_pages, header->segments[1].file_off, header->segments[1].size, PAGE_R);
    mark_pages(kind, num_pages, header->segments[2].file_off,
               (uint64_t)header->segments[2].size + header->bss_size, PAGE_RW);
    /* Alignment gaps stay PAGE_RW (0) as well. */

    /* The kernel forbids W -> X: set RX first, then the read-only pages, then
     * the writable remainder.  Every page is set exactly once, so a segment
     * that shares a page with the next one cannot undo an earlier permission. */
    for (int pass = 0; pass < 3; ++pass) {
        const uint8_t want = pass == 0 ? PAGE_RX : pass == 1 ? PAGE_R : PAGE_RW;
        const u32 perm = want == PAGE_RX ? Perm_Rx : want == PAGE_R ? Perm_R : Perm_Rw;
        size_t pg = 0;
        while (pg < num_pages) {
            if (kind[pg] != want) {
                ++pg;
                continue;
            }
            size_t end = pg;
            while (end < num_pages && kind[end] == want) {
                ++end;
            }
            const uint64_t addr = (uint64_t)(uintptr_t)base + (uint64_t)pg * PAGE;
            const uint64_t size = (uint64_t)(end - pg) * PAGE;
            const Result rc = svcSetProcessMemoryPermission(process, addr, size, perm);
            if (R_FAILED(rc)) {
                fail("loader: %s permission at %" PRIu64 " (0x%08x)",
                     want == PAGE_RX ? "RX" : want == PAGE_R ? "R" : "RW", addr, rc);
            }
            pg = end;
        }
    }
    free(kind);

    const uint64_t text_size = align_up(header->segments[0].size, PAGE);
    armICacheInvalidate((void *)(uintptr_t)((uint64_t)(uintptr_t)base + header->segments[0].file_off),
                        text_size);
    log_line("loader: image %" PRIu64 " pages, text %" PRIu64 ", rodata %" PRIu64,
             (uint64_t)num_pages, text_size,
             align_up(header->segments[1].size, PAGE));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* No console here: libnx's console renderer uses nv and framebufferClose
     * calls nvExit(), which leaves the game's EGL surface unable to present.
     * Keep the loader's report in loader.log only. */
    memcpy(sSavedTls, (u8 *)armGetTls() + 0x100, sizeof(sSavedTls));
    fsdevMountSdmc();
    mkdir("sdmc:/switch", 0777);
    mkdir(APP_DIR, 0777);
    g_log = fopen(LOADER_LOG, "w");
    if (g_log) {
        setvbuf(g_log, NULL, _IOLBF, 0);
    }

    log_line("Emerald NX loader (direct CRT0 entry, no console, 2026-11-25d)");
    log_line("loader: applet type %d, own process handle 0x%x, main thread 0x%x",
             appletGetAppletType(), envGetOwnProcessHandle(), envGetMainThreadHandle());

    const Handle process = envGetOwnProcessHandle();
    if (process == INVALID_HANDLE) {
        fail("loader: no own-process handle; launch as a title takeover from hbmenu");
    }
    if (!envIsSyscallHinted(0x77) || !envIsSyscallHinted(0x73)) {
        fail("loader: the launcher does not permit svcMapProcessCodeMemory/"
             "svcSetProcessMemoryPermission; launch as a title takeover");
    }

    size_t image_size = 0;
    NroHeader header;
    void *image = load_nro_file(GAME_NRO, &image_size, &header);
    /* Do not make the whole image RW first: the kernel forbids a later
     * W -> X transition on code memory.  Set text RX/rodata R/data RW
     * directly, as hbloader does. */
    void *game_base = map_low(process, image, image_size, 0, false, "game image");
    log_line("loader: applying NRO page permissions...");
    set_nro_permissions(process, game_base, &header, image_size);

    log_line("loader: allocating game heap...");
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
    if (g_log) {
        fclose(g_log);
        g_log = NULL;
    }

    /* nx-hbloader closes every service it opened and restores its TLS before
     * handing control to the NRO, so the NRO's libnx sees a clean process:
     * __appInit opens SM, applet, HID, time and FS itself.  Leaving the
     * loader's sessions open made the NRO abort before main. */
    fsdevUnmountAll();
    fsExit();
    appletExit();
    hidExit();
    timeExit();
    setsysExit();
    smExit();
    memcpy((u8 *)armGetTls() + 0x100, sSavedTls, sizeof(sSavedTls));

    /* The NRO starts with a branch to its CRT0 at +0x80. */
    switch_jump_to_nro(entries, (char *)game_base + 0x80, stack_top, process);
    fail("loader: game returned unexpectedly");
    return 0;
}
