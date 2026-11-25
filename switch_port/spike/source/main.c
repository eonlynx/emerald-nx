/*
 * Emerald NX address probe.
 *
 * Built as a normal libnx NRO. The loader maps it below 4 GiB and gives it a
 * low heap/stack; this program proves that code, data, heap, stack and 32-bit
 * function pointers all live in the 32-bit range.
 */
#include <switch.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define APP_DIR "sdmc:/switch/emerald-nx"
#define GAME_LOG APP_DIR "/game.log"
#define LOW_LIMIT 0x100000000ULL

static volatile uint32_t g_marker = 0x12345678;
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

static void probe(void) {
    g_marker += 1;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    mkdir("sdmc:/switch", 0777);
    mkdir(APP_DIR, 0777);
    fsdevMountSdmc();
    g_log = fopen(GAME_LOG, "w");
    if (g_log) {
        setvbuf(g_log, NULL, _IOLBF, 0);
    }

    const uintptr_t main_addr = (uintptr_t)&main;
    const uintptr_t probe_addr = (uintptr_t)&probe;
    const uintptr_t global_addr = (uintptr_t)&g_marker;
    const uintptr_t stack_addr = (uintptr_t)&argc;
    const uintptr_t tls_addr = (uintptr_t)armGetTls();
    void *heap = envGetHeapOverrideAddr();
    void *allocated = malloc(1U << 20);

    if (allocated) {
        memset(allocated, 0x5A, 1U << 20);
    }

    const uint32_t probe32 = (uint32_t)probe_addr;
    const uint32_t marker_before = g_marker;
    ((void (*)(void))(uintptr_t)probe32)();
    const bool call_ok = g_marker == marker_before + 1;

    const bool low_ok = main_addr < LOW_LIMIT && probe_addr < LOW_LIMIT &&
                        global_addr < LOW_LIMIT && stack_addr < LOW_LIMIT &&
                        (uintptr_t)allocated < LOW_LIMIT &&
                        (uintptr_t)heap < LOW_LIMIT &&
                        (uintptr_t)heap + envGetHeapOverrideSize() <= LOW_LIMIT;

    log_line("Emerald NX address probe");
    log_line("game: main=%p probe=%p global=%p stack=%p tls=%p",
             (void *)main_addr, (void *)probe_addr, (void *)global_addr,
             (void *)stack_addr, (void *)tls_addr);
    log_line("game: heap=%p size=%" PRIu64 " allocated=%p",
             heap, envGetHeapOverrideSize(), allocated);
    log_line("game: process=0x%x main-thread=0x%x",
             envGetOwnProcessHandle(), envGetMainThreadHandle());
    log_line("game: 32-bit probe=%08x call=%s", probe32, call_ok ? "PASS" : "FAIL");
    log_line("game: range=%s", low_ok ? "PASS (<4 GiB)" : "FAIL (>=4 GiB)");
    log_line("game: RESULT %s", (low_ok && call_ok) ? "PASS" : "FAIL");

    fflush(g_log);
    consoleUpdate(NULL);
    svcSleepThread(10'000'000'000ULL);

    if (g_log) {
        fclose(g_log);
        g_log = NULL;
    }
    consoleExit(NULL);
    svcExitProcess();
    return 0;
}
