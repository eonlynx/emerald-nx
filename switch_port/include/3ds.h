/*
 * The part of libctru the reused 3DS native units need on Android
 * (3ds_data.c, 3ds_settings.c, 3ds_prof.c, 3ds_scenario.c): types, the
 * system tick, light locks and events, threads. Everything maps onto
 * pthreads and CLOCK_MONOTONIC; nothing here touches graphics or services.
 */
#ifndef ANDROID_SHIM_3DS_H
#define ANDROID_SHIM_3DS_H

#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 Result;
typedef u32 Handle;

#define R_SUCCEEDED(r) ((Result)(r) >= 0)
#define R_FAILED(r) ((Result)(r) < 0)
#define U64_MAX UINT64_MAX
#define CUR_THREAD_HANDLE 0xFFFF8000

/* Horizon's system tick: 19.2 MHz.  Game code divides by this unit. */
#define SYSCLOCK_ARM11 19200000ULL

/* Defined by switch_arm.c without libnx's headers: libnx's libctru-compatible
 * headers disagree with this shim on Result/Handle and that would spread. */
extern u64 armGetSystemTick(void);

static inline u64 svcGetSystemTick(void)
{
    return armGetSystemTick();
}

/* newlib has nanosleep but not the POSIX clock_nanosleep used by the
 * Android frame pacer. */
static inline int clock_nanosleep(clockid_t clockid, int flags,
                                  const struct timespec *req, struct timespec *rem)
{
    struct timespec relative;

    if (!(flags & TIMER_ABSTIME)) {
        return nanosleep(req, rem);
    }
    struct timespec now;
    if (clock_gettime(clockid, &now) != 0) {
        return -1;
    }
    relative.tv_sec = req->tv_sec - now.tv_sec;
    relative.tv_nsec = req->tv_nsec - now.tv_nsec;
    if (relative.tv_nsec < 0) {
        relative.tv_sec -= 1;
        relative.tv_nsec += 1000000000L;
    }
    if (relative.tv_sec < 0) {
        relative.tv_sec = 0;
        relative.tv_nsec = 0;
    }
    return nanosleep(&relative, rem);
}

static inline void svcSleepThread(s64 ns)
{
    struct timespec ts;

    if (ns <= 0)
    {
        sched_yield();
        return;
    }
    ts.tv_sec = (time_t)(ns / 1000000000LL);
    ts.tv_nsec = (long)(ns % 1000000000LL);
    nanosleep(&ts, NULL);
}

static inline Result svcGetThreadPriority(s32 *priority, Handle handle)
{
    (void)handle;
    *priority = 0x30;
    return 0;
}

static inline void svcOutputDebugString(const char *text, int length)
{
    (void)text;
    (void)length;
}

typedef enum { USERBREAK_PANIC = 0, USERBREAK_ASSERT = 1, USERBREAK_USER = 2 } UserBreakType;
static inline void svcBreak(UserBreakType reason)
{
    (void)reason;
    abort();
}

/* The system core's share for the application (ctr_voxel.c's page stream):
 * Android schedules the worker on any core, as if 30% were granted. */
static inline Result APT_GetAppCpuTimeLimit(u32 *percent)
{
    *percent = 30;
    return 0;
}
static inline Result APT_SetAppCpuTimeLimit(u32 percent)
{
    (void)percent;
    return 0;
}

/* Kernel and firmware versions (3ds_prof.c's identity line). */
#define GET_VERSION_MAJOR(v) (((v) >> 24) & 0xFF)
#define GET_VERSION_MINOR(v) (((v) >> 16) & 0xFF)
#define GET_VERSION_REVISION(v) (((v) >> 8) & 0xFF)
static inline u32 osGetKernelVersion(void) { return 0; }
static inline u32 osGetFirmVersion(void) { return 0; }

/* â”€â”€ Light locks â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
 * An s32 like libctru's (CtrLock is one too, and statics start at 1 or 0):
 * 0 or 1 unlocked, 2 locked, 3 locked with waiters. A futex mutex. */
typedef s32 LightLock;

static inline void CtrFutexWait(volatile s32 *addr, s32 value)
{
    (void)addr;
    (void)value;
    sched_yield();
}
static inline void CtrFutexWake(volatile s32 *addr, int count)
{
    (void)addr;
    (void)count;
}
static inline void LightLock_Init(LightLock *lock) { __atomic_store_n(lock, 1, __ATOMIC_RELEASE); }
static inline int LightLock_TryLock(LightLock *lock)
{
    s32 c = __atomic_load_n(lock, __ATOMIC_RELAXED);
    if (c > 1)
        return 1;
    return __atomic_compare_exchange_n(lock, &c, 2, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED) ? 0 : 1;
}
static inline void LightLock_Lock(LightLock *lock)
{
    if (LightLock_TryLock(lock) == 0)
        return;
    while (__atomic_exchange_n(lock, 3, __ATOMIC_ACQUIRE) > 1)
        CtrFutexWait(lock, 3);
}
static inline void LightLock_Unlock(LightLock *lock)
{
    if (__atomic_exchange_n(lock, 1, __ATOMIC_RELEASE) == 3)
        CtrFutexWake(lock, 1);
}

/* â”€â”€ Light events â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */
typedef enum { RESET_ONESHOT = 0, RESET_STICKY = 1, RESET_PULSE = 2 } ResetType;
typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signalled;
    int mode;
} LightEvent;

static inline void LightEvent_Init(LightEvent *e, ResetType mode)
{
    pthread_mutex_init(&e->mutex, NULL);
    pthread_cond_init(&e->cond, NULL);
    e->signalled = false;
    e->mode = mode;
}
static inline void LightEvent_Clear(LightEvent *e)
{
    pthread_mutex_lock(&e->mutex);
    e->signalled = false;
    pthread_mutex_unlock(&e->mutex);
}
static inline void LightEvent_Signal(LightEvent *e)
{
    pthread_mutex_lock(&e->mutex);
    e->signalled = true;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->mutex);
}
static inline void LightEvent_Wait(LightEvent *e)
{
    pthread_mutex_lock(&e->mutex);
    while (!e->signalled)
        pthread_cond_wait(&e->cond, &e->mutex);
    if (e->mode == RESET_ONESHOT)
        e->signalled = false;
    pthread_mutex_unlock(&e->mutex);
}
/* 1 if it timed out, 0 if signalled. */
static inline int LightEvent_WaitTimeout(LightEvent *e, s64 ns)
{
    struct timespec until;
    int timedOut = 0;

    clock_gettime(CLOCK_REALTIME, &until);
    until.tv_sec += (time_t)(ns / 1000000000LL);
    until.tv_nsec += (long)(ns % 1000000000LL);
    if (until.tv_nsec >= 1000000000L)
    {
        until.tv_sec++;
        until.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&e->mutex);
    while (!e->signalled && !timedOut)
        timedOut = pthread_cond_timedwait(&e->cond, &e->mutex, &until) != 0;
    if (e->signalled && e->mode == RESET_ONESHOT)
        e->signalled = false;
    pthread_mutex_unlock(&e->mutex);
    return timedOut && !e->signalled;
}


/* â”€â”€ Threads â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */
typedef struct AndroidThread *Thread;
typedef void (*ThreadFunc)(void *);
/* libctru's names, implemented on pthreads with low stacks by
 * android_platform.c.  Prefix them so libnx's own threadCreate (used by its
 * newlib layer, and with a different signature) is not shadowed. */
Thread CtrThreadCreate(ThreadFunc entry, void *arg, size_t stackSize, int prio, int core, bool detached);
Result CtrThreadJoin(Thread thread, u64 timeoutNs);
void CtrThreadFree(Thread thread);
#define threadCreate CtrThreadCreate
#define threadJoin CtrThreadJoin
#define threadFree CtrThreadFree

/* â”€â”€ Local wireless (UDS) over Bluetooth LE â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */
#include "3ds_uds.h"

#endif
