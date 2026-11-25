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

    const AppletOperationMode mode = appletGetOperationMode();
    const int width = mode == AppletOperationMode_Console ? 1920 : 1280;
    const int height = mode == AppletOperationMode_Console ? 1080 : 720;
    AndroidVideo_ChooseWidth(width, height);
    AndroidVideo_SetSingleScreen(true);

    if (!CtrPlatform_Init()) {
        CtrPlatform_Fatal("platform initialization failed");
    }

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
