#ifndef SWITCH_LIFECYCLE_H
#define SWITCH_LIFECYCLE_H

#include <stdbool.h>

/* appletMainLoop: processes HOME/exit/sleep/dock notifications and returns
 * false once the application should leave.  Focused is false while the HOME
 * menu, a library applet or console sleep owns the screen. */
bool SwitchLifecycle_Frame(void);
bool SwitchLifecycle_Focused(void);

#endif
