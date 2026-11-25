/*
 * Applet lifecycle, in its own unit: <switch.h> defines Result/Handle with
 * libnx's types, which cannot be mixed with the libctru-compatible shim the
 * rest of the game uses.  The host layer only sees the two bools.
 */
#include <switch.h>

#include "switch_lifecycle.h"

static AppletHookCookie sCookie;
static volatile bool sInitialized;
static volatile bool sFocused = true;
static volatile bool sExitRequested;

static void LifecycleHook(AppletHookType type, void *param)
{
    (void)param;
    if (type == AppletHookType_OnFocusState) {
        sFocused = appletGetFocusState() == AppletFocusState_InFocus;
    } else if (type == AppletHookType_OnExitRequest) {
        sExitRequested = true;
    }
}

static void Initialize(void)
{
    if (sInitialized) {
        return;
    }
    sFocused = appletGetFocusState() == AppletFocusState_InFocus;
    appletHook(&sCookie, LifecycleHook, NULL);
    sInitialized = true;
}

bool SwitchLifecycle_Frame(void)
{
    Initialize();
    if (!appletMainLoop()) {
        sExitRequested = true;
    }
    return !sExitRequested;
}

bool SwitchLifecycle_Focused(void)
{
    Initialize();
    return sFocused && appletGetFocusState() == AppletFocusState_InFocus;
}
