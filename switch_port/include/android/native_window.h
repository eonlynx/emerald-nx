#ifndef SWITCH_SHIM_NATIVE_WINDOW_H
#define SWITCH_SHIM_NATIVE_WINDOW_H

#include <stdint.h>

typedef struct NWindow NWindow;

NWindow *nwindowGetDefault(void);
void nwindowSetDimensions(NWindow *window, int width, int height);

typedef enum {
    AppletOperationMode_Handheld = 0,
    AppletOperationMode_Console = 1,
} AppletOperationMode;

AppletOperationMode appletGetOperationMode(void);

/* The one NWindow the game renders into. */
struct ANativeWindow {
    NWindow *nwindow;
    int width;
    int height;
};

typedef struct ANativeWindow ANativeWindow;

static inline void ANativeWindow_acquire(ANativeWindow *window) { (void)window; }
static inline void ANativeWindow_release(ANativeWindow *window) { (void)window; }
static inline int ANativeWindow_getWidth(ANativeWindow *window) { return window ? window->width : 1280; }
static inline int ANativeWindow_getHeight(ANativeWindow *window) { return window ? window->height : 720; }
static inline void ANativeWindow_setFrameRate(ANativeWindow *window, float rate, int compatibility)
{
    (void)window; (void)rate; (void)compatibility;
}
#define ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_FIXED_SOURCE 0

#endif
