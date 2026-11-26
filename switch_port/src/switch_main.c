/*
 * Emerald NX entry point.
 *
 * The loader has already mapped this NRO and its heap/stack below 4 GiB and
 * supplied the libnx ConfigEntry table.  From here the game runs the real
 * Emerald engine through the Android single-screen backend adapted to Horizon.
 */
#include <switch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "android_host.h"
#include "3ds_platform.h"
#include "3ds_video.h"

#define APP_DIR "sdmc:/switch/emerald-nx"
#define LOG_PATH APP_DIR "/emerald-nx.log"

/* libnx only sends the applet's exit commands for an NSO by default.  This
 * is an NRO running in a title takeover: without them Horizon reports
 * "the software was closed because an error occurred" on the way back to
 * the launcher.  Mode 1 runs them for an NRO too. */
u32 __nx_applet_exit_mode = 1;

/* libnx's own exit path: runs __appExit (which arms the applet exit) and
 * then the loader's exit function.  newlib's exit()/_exit() bypasses it, and
 * svcExitProcess skips the applet's exit command entirely. */
void __libnx_exit(int rc) __attribute__((noreturn));

void CtrPlatform_ExitProcess(void) {
    CtrPlatform_Shutdown();
    __libnx_exit(0);
}

/* stderr may not exist yet when the game starts; write a breadcrumb through
 * the FS service, as Dusklight's exit trace does, so the last reached stage
 * is readable even if stdio never opens. */
#define BOOT_LOG_MAX 4096

static void early_log(const char *line) {
    static bool boot_log_started;
    FsFileSystem fs;
    FsFile file;
    s64 size = 0;
    const char *path = "/switch/emerald-nx/boot.log";

    if (R_FAILED(fsOpenSdCardFileSystem(&fs))) {
        return;
    }
    fsFsCreateDirectory(&fs, "/switch");
    fsFsCreateDirectory(&fs, "/switch/emerald-nx");
    /* boot.log is only the breadcrumb of this launch: recreate it on the first
     * line, so repeated launches cannot grow it. */
    if (!boot_log_started) {
        boot_log_started = true;
        fsFsDeleteFile(&fs, path);
        fsFsCreateFile(&fs, path, 0, 0);
    }
    if (R_SUCCEEDED(fsFsOpenFile(&fs, path, FsOpenMode_Write | FsOpenMode_Append, &file))) {
        fsFileGetSize(&file, &size);
        if (size + (s64)strlen(line) <= BOOT_LOG_MAX) {
            fsFileWrite(&file, size, line, strlen(line), FsWriteOption_Flush);
        }
        fsFileClose(&file);
    }
    fsFsClose(&fs);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    /* Breadcrumbs through the FS service, before stdio needs a mounted sdmc:
     * if the process dies before the log exists, these say how far it got. */
    early_log("Emerald NX: main entered\n");
    fsdevMountSdmc();
    early_log("Emerald NX: sdmc mounted\n");
    mkdir("sdmc:/switch", 0777);
    mkdir(APP_DIR, 0777);
    mkdir(APP_DIR "/data", 0777);

    /* One rotation per launch: the previous run stays available for support,
     * and the current log is capped in switch_log.c. */
    remove(APP_DIR "/emerald-nx-prev.log");
    rename(LOG_PATH, APP_DIR "/emerald-nx-prev.log");
    if (freopen(LOG_PATH, "w", stderr)) {
        setvbuf(stderr, NULL, _IOLBF, 0);
        early_log("Emerald NX: emerald-nx.log opened\n");
    }
    fprintf(stderr, "Emerald NX: log capped at 1 MiB, previous run at emerald-nx-prev.log\n");
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
