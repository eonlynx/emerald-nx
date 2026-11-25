/*
 * Applet lifecycle, in its own unit: <switch.h> defines Result/Handle with
 * libnx's types, which cannot be mixed with the libctru-compatible shim the
 * rest of the game uses.  The host layer only sees the two bools.
 */
#include <switch.h>

#include "switch_lifecycle.h"

bool SwitchLifecycle_Frame(void)
{
    return appletMainLoop();
}

bool SwitchLifecycle_Focused(void)
{
    return appletGetFocusState() == AppletFocusState_InFocus;
}
