/*
 * Host services shared by the Android backend sources adapted for Switch:
 * paths, lifecycle and the window stubs.  The actual window is the default
 * libnx NWindow; android_video.c creates its EGL surface directly.
 */
#include <switch.h>

#include <stdbool.h>
#include <stdint.h>

#include "android_host.h"

#define APP_DIR "sdmc:/switch/emerald-nx"

struct ANativeWindow *AndroidHost_AcquireWindow(int screen, uint32_t *generation) {
    (void)screen;
    if (generation) {
        *generation = 0;
    }
    return NULL;
}

void AndroidHost_ReleaseWindow(struct ANativeWindow *window) { (void)window; }
void AndroidHost_WindowDone(int screen, uint32_t generation) { (void)screen; (void)generation; }

const char *AndroidHost_FilesDir(void) { return APP_DIR; }
const char *AndroidHost_RomfsDir(void) { return "data"; }

bool AndroidHost_Paused(void) { return false; }
void AndroidHost_WaitResumed(void) {}
uint32_t AndroidHost_ResumeCount(void) { return 0; }
bool AndroidHost_TakeCaptureRequest(void) { return false; }
