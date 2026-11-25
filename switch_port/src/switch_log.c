/*
 * The port's log on Android: every line to logcat (tag "Emerald"), and, as on
 * the 3DS, to sdmc:/3ds/emerald3ds/port.log when debug.txt exists there. The
 * file is written by its own thread so the game never waits on storage.
 */
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "3ds_log.h"
#include "3ds_platform.h"

#define LOG_DIR "sdmc:/3ds/emerald3ds/"
#define RING_BYTES (256u << 10)

static const char *const sTags[] = { "BOOT", "VIDEO", "INPUT", "FS", "AUDIO", "GAME", "ERROR" };
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
static bool sFileOn;
static FILE *sFile;
static char sFatal[256];

bool CtrLog_Enabled(void) { return sFileOn; }

void CtrLog_LastDrain(float *ms, float *agoMs)
{
    *ms = 0.0f;
    *agoMs = 1.0e6f;
}

void CtrLog_Init(void)
{
    fprintf(stderr, "Emerald NX: log init (mkdir)\n");
    fflush(stderr);
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/emerald3ds", 0777);
    fprintf(stderr, "Emerald NX: log init (open)\n");
    fflush(stderr);
    rename(LOG_DIR "port.log", LOG_DIR "port-prev.log");
    sFile = fopen(LOG_DIR "port.log", "w");
    sFileOn = sFile != NULL;
    fprintf(stderr, "Emerald NX: log started (file %s)\n", sFileOn ? "on" : "off");
    fflush(stderr);
}

void CtrLog_Close(void)
{
    if (!sFileOn)
        return;
    pthread_mutex_lock(&sLock);
    fclose(sFile);
    sFile = NULL;
    sFileOn = false;
    pthread_mutex_unlock(&sLock);
}

void CtrLog_Write(CtrLogCategory category, const char *format, ...)
{
    char line[1024];
    va_list args;
    int prefix, length;
    unsigned cat = (unsigned)category < sizeof(sTags) / sizeof(sTags[0]) ? (unsigned)category : 6;
    uint64_t ms = CtrPlatform_Milliseconds();

    prefix = snprintf(line, sizeof(line), "[%6llu.%03llu] [%s] ", (unsigned long long)(ms / 1000),
                      (unsigned long long)(ms % 1000), sTags[cat]);
    va_start(args, format);
    length = vsnprintf(line + prefix, sizeof(line) - (size_t)prefix - 1, format, args);
    va_end(args);
    if (length < 0)
        return;
    length += prefix;
    if (length > (int)sizeof(line) - 2)
        length = (int)sizeof(line) - 2;
    while (length > 0 && line[length - 1] == '\n')
        length--;
    line[length] = 0;
    fprintf(stderr, "%s\n", line + prefix);
    fflush(stderr);
    if (sFileOn)
    {
        line[length++] = '\n';
        pthread_mutex_lock(&sLock);
        if (sFile)
        {
            fwrite(line, 1, (size_t)length, sFile);
            fflush(sFile);
        }
        pthread_mutex_unlock(&sLock);
    }
}

void CtrLog_WriteBinary(const void *data, unsigned size)
{
    (void)data;
    (void)size;
}

/* The reason is kept for the video backend to show (android_video.c). */
void CtrLog_ShowFatal(const char *reason)
{
    snprintf(sFatal, sizeof(sFatal), "%s", reason ? reason : "fatal error");
    fprintf(stderr, "Emerald NX FATAL: %s\n", sFatal);
    fflush(stderr);
}

const char *AndroidLog_FatalReason(void) { return sFatal[0] ? sFatal : NULL; }
