/*
 * Emerald NX entry point.
 *
 * The loader has already mapped this NRO and its heap/stack below 4 GiB and
 * supplied the libnx ConfigEntry table.  From here the game runs the real
 * Emerald engine through the Android single-screen backend adapted to Horizon.
 */
#include <switch.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "android_host.h"
#include "3ds_platform.h"
#include "3ds_video.h"

#define APP_DIR "sdmc:/switch/emerald-nx"
#define LOG_PATH APP_DIR "/emerald-nx.log"

/* libnx's crash path writes the faulting context here, as smbr-nx does.  The
 * report goes to emerald-nx.log before the process is closed, so a first-boot
 * fault is diagnosable from the SD card. */
u8 __nx_exception_stack[0x8000] __attribute__((aligned(16)));
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    fprintf(stderr,
            "Emerald NX CRASH: desc=0x%x PC=%016llx LR=%016llx SP=%016llx "
            "FAR=%016llx ESR=%08x\n",
            ctx->error_desc,
            (unsigned long long)ctx->pc.x,
            (unsigned long long)ctx->lr.x,
            (unsigned long long)ctx->sp.x,
            (unsigned long long)ctx->far.x,
            ctx->esr);
    fflush(stderr);
    svcExitProcess();
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    fsdevMountSdmc();
    mkdir("sdmc:/switch", 0777);
    mkdir(APP_DIR, 0777);
    mkdir(APP_DIR "/data", 0777);

    if (freopen(LOG_PATH, "w", stderr)) {
        setvbuf(stderr, NULL, _IOLBF, 0);
    }
    fprintf(stderr, "Emerald NX: starting (applet %d)\n", appletGetAppletType());
    fflush(stderr);

    fprintf(stderr, "Emerald NX: operation mode\n");
    fflush(stderr);
    const AppletOperationMode mode = appletGetOperationMode();
    const int width = mode == AppletOperationMode_Console ? 1920 : 1280;
    const int height = mode == AppletOperationMode_Console ? 1080 : 720;

    fprintf(stderr, "Emerald NX: video size %dx%d\n", width, height);
    fflush(stderr);
    AndroidVideo_ChooseWidth(width, height);
    AndroidVideo_SetSingleScreen(true);
    fprintf(stderr, "Emerald NX: video configured\n");
    fflush(stderr);

    fprintf(stderr, "Emerald NX: platform init\n");
    fflush(stderr);
    if (!CtrPlatform_Init()) {
        CtrPlatform_Fatal("platform initialization failed");
    }
    fprintf(stderr, "Emerald NX: platform initialized\n");
    fflush(stderr);

    CtrPlatformHooks hooks = {0};
    hooks.vblank = CtrGame_VBlank;
    hooks.videoPresent = CtrVideo_Present;
    CtrPlatform_SetHooks(&hooks);

    if (!CtrPlatform_BeginFrame()) {
        CtrPlatform_Shutdown();
        return 0;
    }

    /* AgbMain owns the loop; this returns only through the platform's exit
     * path (soft reset or quit). */
    CtrGame_Init();
    CtrPlatform_Shutdown();
    return 0;
}
