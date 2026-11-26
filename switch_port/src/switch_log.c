/*
 * The port's log on Switch: stderr, redirected by switch_main.c to
 * sdmc:/switch/emerald-nx/emerald-nx.log.  Each launch starts a fresh file and
 * keeps the previous run as emerald-nx-prev.log; a run is capped at 1 MiB and
 * then stops writing.  Verbose diagnostics (registers, tilemaps, per-frame
 * timings, asset reads) are off unless sdmc:/switch/emerald-nx/debug.txt
 * exists.
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
#define APP_DIR "sdmc:/switch/emerald-nx"
#define RING_BYTES (256u << 10)
/* A run never writes more than this to emerald-nx.log.  Together with the one
 * rotated previous log, the port can use at most 2 MiB of the SD card. */
#define LOG_MAX_BYTES (1024u * 1024u)

static const char *const sTags[] = { "BOOT", "VIDEO", "INPUT", "FS", "AUDIO", "GAME", "ERROR" };
static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;
static bool sFileOn;
static bool sVerbose;
static bool sLogCapped;
static FILE *sFile;
static char sFatal[256];

bool CtrLog_Enabled(void) { return sFileOn; }
bool CtrLog_Verbose(void) { return sVerbose; }

void CtrLog_LastDrain(float *ms, float *agoMs)
{
    *ms = 0.0f;
    *agoMs = 1.0e6f;
}

void CtrLog_Init(void)
{
    struct stat st;

    fprintf(stderr, "Emerald NX: log init (mkdir)\n");
    fflush(stderr);
    mkdir("sdmc:/3ds", 0777);
    mkdir("sdmc:/3ds/emerald3ds", 0777);
    /* stderr is already redirected to emerald-nx.log by switch_main; a
     * second port.log file and its pthread writer are not needed. */
    sFile = NULL;
    sFileOn = false;
    sVerbose = stat(APP_DIR "/debug.txt", &st) == 0;
    sLogCapped = false;
    fprintf(stderr, "Emerald NX: log started (stderr only, verbose %s)\n",
            sVerbose ? "on (debug.txt)" : "off");
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
    va_list args;
    unsigned cat = (unsigned)category < sizeof(sTags) / sizeof(sTags[0]) ? (unsigned)category : 6;
    /* stderr is the log; the physical counter is safe before CtrLog_Init and
     * makes the lines' times useful. */
    const uint64_t ms = CtrPlatform_Milliseconds();
    const long used = ftell(stderr);

    if (sLogCapped)
        return;
    if (used >= (long)LOG_MAX_BYTES)
    {
        sLogCapped = true;
        fprintf(stderr,
                "Emerald NX: log limit reached (%u KiB); further output is suppressed for this run\n",
                LOG_MAX_BYTES >> 10);
        fflush(stderr);
        return;
    }

    fprintf(stderr, "[%6llu.%03llu] [%s] ", (unsigned long long)(ms / 1000),
            (unsigned long long)(ms % 1000), sTags[cat]);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    fflush(stderr);

    if (sFileOn)
    {
        pthread_mutex_lock(&sLock);
        if (sFile)
        {
            fprintf(sFile, "[%6llu.%03llu] [%s] ", (unsigned long long)(ms / 1000),
                    (unsigned long long)(ms % 1000), sTags[cat]);
            va_start(args, format);
            vfprintf(sFile, format, args);
            va_end(args);
            fputc('\n', sFile);
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
