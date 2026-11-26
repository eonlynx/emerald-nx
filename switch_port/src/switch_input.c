/*
 * Switch input: libnx pad/touch mapped onto the AndroidHostInput surface the
 * reused Android input decoder expects.  Buttons are mapped by their printed
 * letters (A=A, B=B, X=X, Y=Y), as toniisound does in the Oracle ports.
 */
#include <switch.h>

#include <stdbool.h>
#include <stdint.h>

#include "android_host.h"
#include "3ds_log.h"

static PadState sPad;
static bool sReady;
static bool sTouchInPanel;

static void ensure_ready(void) {
    if (sReady) {
        return;
    }
    padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&sPad);
    hidInitializeTouchScreen();
    sReady = true;
}

void AndroidHost_GetInput(AndroidHostInput *out) {
    ensure_ready();
    padUpdate(&sPad);

    const u64 held = padGetButtons(&sPad);
    static u64 sLastHeld;
    if (held != sLastHeld) {
        CtrLog_Write(CTR_LOG_INPUT, "pad buttons 0x%016llx", (unsigned long long)held);
        sLastHeld = held;
    }
    out->buttons = 0;
    if (held & HidNpadButton_A) out->buttons |= HOST_BTN_A;
    if (held & HidNpadButton_B) out->buttons |= HOST_BTN_B;
    if (held & HidNpadButton_X) out->buttons |= HOST_BTN_X;
    if (held & HidNpadButton_Y) out->buttons |= HOST_BTN_Y;
    if (held & HidNpadButton_L) out->buttons |= HOST_BTN_L;
    if (held & HidNpadButton_R) out->buttons |= HOST_BTN_R;
    if (held & HidNpadButton_ZL) out->buttons |= HOST_BTN_ZL;
    if (held & HidNpadButton_ZR) out->buttons |= HOST_BTN_ZR;
    if (held & HidNpadButton_Plus) out->buttons |= HOST_BTN_START;
    if (held & HidNpadButton_Minus) out->buttons |= HOST_BTN_SELECT;
    if (held & HidNpadButton_Up) out->buttons |= HOST_BTN_UP;
    if (held & HidNpadButton_Down) out->buttons |= HOST_BTN_DOWN;
    if (held & HidNpadButton_Left) out->buttons |= HOST_BTN_LEFT;
    if (held & HidNpadButton_Right) out->buttons |= HOST_BTN_RIGHT;

    const HidAnalogStickState stick = padGetStickPos(&sPad, 0);
    out->stickX = (float)stick.x / 32768.0f;
    out->stickY = (float)stick.y / 32768.0f;

    out->touchActive = false;
    out->touchX = out->touchY = 0.0f;
    out->touchW = appletGetOperationMode() == AppletOperationMode_Console ? 1920 : 1280;
    out->touchH = appletGetOperationMode() == AppletOperationMode_Console ? 1080 : 720;

    if (appletGetOperationMode() != AppletOperationMode_Console) {
        HidTouchScreenState touch = {0};
        const int screenW = 1280;
        const int screenH = 720;
        int px, py, pw, ph;

        AndroidVideo_PanelRect(screenW, screenH, &px, &py, &pw, &ph);
        if (hidGetTouchScreenStates(&touch, 1) && touch.count > 0) {
            const float x = (float)touch.touches[0].x;
            const float y = (float)touch.touches[0].y;

            /* A touch starts on the bottom screen only when it lands in the
             * panel drawn on the single top screen; it is followed until it
             * is released, as android_main.c does with nativeTouchTop. */
            if (!sTouchInPanel) {
                sTouchInPanel = AndroidVideo_PanelShown() &&
                                x >= (float)px && x < (float)(px + pw) &&
                                y >= (float)py && y < (float)(py + ph);
            }
            if (sTouchInPanel) {
                out->touchActive = true;
                out->touchX = x - (float)px;
                out->touchY = y - (float)py;
                out->touchW = pw;
                out->touchH = ph;
            }
        } else {
            sTouchInPanel = false;
        }
    } else {
        sTouchInPanel = false;
    }
}
