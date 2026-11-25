/*
 * Local wireless is not part of the first Switch milestone.  The game's
 * adapter and UI still call these entry points, so answer as the 3DS backend
 * does when no session exists.
 */
#include <stdint.h>
#include <string.h>

#include "3ds_local.h"
#include "3ds_platform.h"

void LocalLink_Init(void) {}
void LocalLink_Shutdown(void) {}
uint8_t LocalLink_Enabled(void) { return 0; }
uint16_t LocalLink_MyId(void) { return 0; }
uint32_t LocalLink_Generation(void) { return 0; }
void LocalLink_SetAdvert(uint8_t hosting, uint8_t nextSlot, const uint8_t *data)
{
    (void)hosting;
    (void)nextSlot;
    (void)data;
}
int LocalLink_GetPeers(struct LocalPeer *out, int max) { (void)out; (void)max; return 0; }
void LocalLink_Send(uint16_t dest, const void *data, uint32_t len) { (void)dest; (void)data; (void)len; }
uint8_t LocalLink_PollEvent(struct LocalEvent *ev) { (void)ev; return 0; }
uint32_t LocalLink_Milliseconds(void) { return (uint32_t)CtrPlatform_Milliseconds(); }
void LocalLink_StartSession(uint8_t kind) { (void)kind; }
void LocalLink_LeaveSession(void) {}
uint8_t LocalLink_GetSession(void) { return 0; }
uint8_t LocalLink_GetSessionKind(void) { return 0; }
uint8_t LocalLink_InSession(void) { return 0; }
void LocalLink_LeaveBeforeSleep(void) {}
void LocalLink_AfterWake(void) {}
