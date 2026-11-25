/*
 * Switch audio: the Android backend's 42060 Hz mixer ring and linear
 * resampler, fed to Horizon's audout service at 48000 Hz.
 *
 * The game mixes 701 stereo frames per 60 Hz frame on its own thread.  A
 * feeder thread resamples to the device rate and keeps a few 1024-frame
 * audout buffers in flight.  This mirrors smbr-nx's audout feeder and keeps
 * the Android ring/drift logic.
 */
#include <switch.h>

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "3ds_audio.h"
#include "3ds_platform.h"

#define RING_FRAMES 8192u
#define RING_MASK (RING_FRAMES - 1u)
#define TARGET_FRAMES (CTR_AUDIO_TARGET_DEPTH * 701.0f)
#define OUT_FRAMES 1024u
#define OUT_BYTES (OUT_FRAMES * 2u * sizeof(int16_t))
#define BUFFERS 4
#define TRIM 0.0025f
#define TRIM_MAX 0.0100f

typedef struct {
    AudioOutBuffer buffer;
    int16_t *data;
    bool busy;
} OutBuffer;

static int16_t sRing[RING_FRAMES * 2];
static atomic_uint sWritten, sRead;
static OutBuffer sOut[BUFFERS];
static pthread_t sFeeder;
static atomic_bool sQuit;
static atomic_bool sPaused;
static bool sStarted;
static float sPhase;
static unsigned sOutRate = 48000;
static CtrAudioStats sStats;

CtrAudioStats *CtrAudio_Stats(void) { return &sStats; }

static void reclaim(void) {
    for (;;) {
        AudioOutBuffer *released = NULL;
        u32 count = 0;
        if (R_FAILED(audoutGetReleasedAudioOutBuffer(&released, &count)) || released == NULL) {
            break;
        }
        for (int i = 0; i < BUFFERS; ++i) {
            if (&sOut[i].buffer == released) {
                sOut[i].busy = false;
                break;
            }
        }
        if (count <= 1) {
            break;
        }
    }
}

static void *Feeder(void *arg) {
    (void)arg;
    while (!atomic_load_explicit(&sQuit, memory_order_acquire)) {
        if (atomic_load_explicit(&sPaused, memory_order_acquire)) {
            svcSleepThread(5000000ULL);
            continue;
        }

        reclaim();
        int slot = -1;
        for (int i = 0; i < BUFFERS; ++i) {
            if (!sOut[i].busy) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            svcSleepThread(1000000ULL);
            continue;
        }

        unsigned read = atomic_load_explicit(&sRead, memory_order_acquire);
        unsigned written = atomic_load_explicit(&sWritten, memory_order_acquire);
        unsigned avail = written - read;
        const float trim = ((float)avail - TARGET_FRAMES) / 701.0f * TRIM;
        const float clamped = trim > TRIM_MAX ? TRIM_MAX : trim < -TRIM_MAX ? -TRIM_MAX : trim;
        const float step = (float)CTR_AUDIO_SAMPLE_RATE / (float)sOutRate * (1.0f + clamped);
        int frames = 0;
        int16_t *out = sOut[slot].data;

        while (frames < (int)OUT_FRAMES && avail >= 2) {
            const int16_t *a = &sRing[(read & RING_MASK) * 2];
            const int16_t *b = &sRing[((read + 1) & RING_MASK) * 2];
            out[frames * 2 + 0] = (int16_t)(a[0] + (b[0] - a[0]) * sPhase);
            out[frames * 2 + 1] = (int16_t)(a[1] + (b[1] - a[1]) * sPhase);
            sPhase += step;
            while (sPhase >= 1.0f && avail >= 2) {
                sPhase -= 1.0f;
                ++read;
                --avail;
            }
            ++frames;
        }
        atomic_store_explicit(&sRead, read, memory_order_release);

        if (frames == 0) {
            svcSleepThread(1000000ULL);
            continue;
        }

        sOut[slot].buffer.data_size = (u64)frames * 2u * sizeof(int16_t);
        sOut[slot].buffer.data_offset = 0;
        sOut[slot].buffer.next = NULL;
        const Result rc = audoutAppendAudioOutBuffer(&sOut[slot].buffer);
        if (R_SUCCEEDED(rc)) {
            sOut[slot].busy = true;
            sStats.rateHz = (float)sOutRate / step;
        } else {
            ++sStats.drops;
        }
    }
    return NULL;
}

static bool OpenAudio(void) {
    Result rc = audoutInitialize();
    if (R_FAILED(rc)) {
        CtrLog_Write(CTR_LOG_AUDIO, "audoutInitialize: 0x%08x", rc);
        return false;
    }

    rc = audoutStartAudioOut();
    if (R_FAILED(rc)) {
        CtrLog_Write(CTR_LOG_AUDIO, "audoutStartAudioOut: 0x%08x", rc);
        audoutExit();
        return false;
    }
    sOutRate = audoutGetSampleRate();
    const u32 channels = audoutGetChannelCount();
    const PcmFormat format = audoutGetPcmFormat();
    if (sOutRate == 0) {
        sOutRate = 48000;
    }

    for (int i = 0; i < BUFFERS; ++i) {
        memset(&sOut[i], 0, sizeof(sOut[i]));
        sOut[i].data = memalign(0x1000, OUT_BYTES);
        if (!sOut[i].data) {
            CtrLog_Write(CTR_LOG_AUDIO, "audio: out of memory");
            audoutExit();
            return false;
        }
        memset(sOut[i].data, 0, OUT_BYTES);
        sOut[i].buffer.buffer = sOut[i].data;
        sOut[i].buffer.buffer_size = OUT_BYTES;
        sOut[i].buffer.data_size = 0;
        sOut[i].buffer.data_offset = 0;
        sOut[i].buffer.next = NULL;
    }

    CtrLog_Write(CTR_LOG_AUDIO, "audout: %u Hz, %u channels (format %d), mixer %d Hz",
                 sOutRate, channels, (int)format, CTR_AUDIO_SAMPLE_RATE);
    return true;
}

bool CtrAudio_Init(void) {
    if (!OpenAudio()) {
        return false;
    }
    atomic_store(&sQuit, false);
    atomic_store(&sPaused, false);
    if (pthread_create(&sFeeder, NULL, Feeder, NULL) != 0) {
        CtrLog_Write(CTR_LOG_AUDIO, "audio: feeder thread failed");
        audoutExit();
        return false;
    }
    sStarted = true;
    return true;
}

void CtrAudio_Shutdown(void) {
    if (!sStarted) {
        return;
    }
    atomic_store(&sQuit, true);
    pthread_join(sFeeder, NULL);
    sStarted = false;
    audoutStopAudioOut();
    for (int i = 0; i < BUFFERS; ++i) {
        free(sOut[i].data);
        sOut[i].data = NULL;
    }
    audoutExit();
}

void AndroidAudio_SetPaused(bool paused) {
    atomic_store(&sPaused, paused);
}

void CtrAudio_Queue(const int32_t *interleaved, int frames) {
    if (!sStarted || !interleaved || frames <= 0) {
        return;
    }
    unsigned written = atomic_load_explicit(&sWritten, memory_order_relaxed);
    const unsigned read = atomic_load_explicit(&sRead, memory_order_acquire);
    unsigned room = RING_FRAMES - 1u - (written - read);
    if ((unsigned)frames > room) {
        ++sStats.drops;
        return;
    }

    int32_t peak = 0;
    for (int i = 0; i < frames; ++i) {
        int16_t *dst = &sRing[((written + (unsigned)i) & RING_MASK) * 2];
        for (int c = 0; c < 2; ++c) {
            int32_t sample = (interleaved[i * 2 + c] + 128) >> 8;
            if (sample > 32767) sample = 32767;
            else if (sample < -32768) sample = -32768;
            dst[c] = (int16_t)sample;
            const int32_t magnitude = sample < 0 ? -sample : sample;
            if (magnitude > peak) peak = magnitude;
        }
    }
    atomic_store_explicit(&sWritten, written + (unsigned)frames, memory_order_release);
    ++sStats.frames;
    sStats.peak = (uint32_t)peak;
    sStats.queued = (written + (unsigned)frames - read) / 701u;
}

bool CtrAudio_StartWorker(void (*mix)(void)) { (void)mix; return false; }
bool CtrAudio_Kick(void) { return false; }
void CtrAudio_SetClock(bool on) { (void)on; }
void CtrAudio_LockSound(void) {}
void CtrAudio_UnlockSound(void) {}
