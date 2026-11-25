/*
 * Switch filesystem: map the 3DS paths used throughout the shared sources
 * onto the SD-card layout:
 *
 *   romfs:/...                    -> sdmc:/switch/emerald-nx/data/...
 *   sdmc:/3ds/emerald3ds/...      -> sdmc:/switch/emerald-nx/...
 *   sdmc:/3ds/pokeemerald/...     -> sdmc:/switch/emerald-nx/legacy/...
 *
 * The path translation is done with link-time wrappers so the game and the
 * port's shared units keep their original 3DS paths.
 */
#include <3ds.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "3ds_platform.h"

#define APP_DIR "sdmc:/switch/emerald-nx"
#define ROMFS_PREFIX "romfs:/"
#define SD_OLD_PREFIX "sdmc:/3ds/emerald3ds"
#define SD_LEGACY_PREFIX "sdmc:/3ds/pokeemerald"
#define SD_3DS_PREFIX "sdmc:/3ds"

static const char *Translate(const char *path, char *out, size_t size) {
    const char *rest;
    const char *base;

    if (!path) {
        return NULL;
    }
    if (strncmp(path, ROMFS_PREFIX, sizeof(ROMFS_PREFIX) - 1) == 0) {
        rest = path + sizeof(ROMFS_PREFIX) - 1;
        base = APP_DIR "/data/";
    } else if (strncmp(path, SD_OLD_PREFIX, sizeof(SD_OLD_PREFIX) - 1) == 0) {
        rest = path + sizeof(SD_OLD_PREFIX) - 1;
        base = APP_DIR;
    } else if (strncmp(path, SD_LEGACY_PREFIX, sizeof(SD_LEGACY_PREFIX) - 1) == 0) {
        rest = path + sizeof(SD_LEGACY_PREFIX) - 1;
        base = APP_DIR "/legacy";
    } else if (strncmp(path, SD_3DS_PREFIX, sizeof(SD_3DS_PREFIX) - 1) == 0) {
        rest = path + sizeof(SD_3DS_PREFIX) - 1;
        base = APP_DIR;
    } else {
        return NULL;
    }
    if (snprintf(out, size, "%s%s", base, rest) >= (int)size) {
        return NULL;
    }
    return out;
}

#define MAPPED(path) \
    char mapped_##path[768]; \
    const char *t_##path = Translate(path, mapped_##path, sizeof(mapped_##path)); \
    if (t_##path) path = t_##path

FILE *__real_fopen(const char *path, const char *mode);
FILE *__wrap_fopen(const char *path, const char *mode) {
    MAPPED(path);
    return __real_fopen(path, mode);
}

int __real_rename(const char *from, const char *to);
int __wrap_rename(const char *from, const char *to) {
    MAPPED(from);
    MAPPED(to);
    return __real_rename(from, to);
}

int __real_remove(const char *path);
int __wrap_remove(const char *path) {
    MAPPED(path);
    return __real_remove(path);
}

int __real_unlink(const char *path);
int __wrap_unlink(const char *path) {
    MAPPED(path);
    return __real_unlink(path);
}

int __real_mkdir(const char *path, mode_t mode);
int __wrap_mkdir(const char *path, mode_t mode) {
    MAPPED(path);
    return __real_mkdir(path, mode);
}

int __real_stat(const char *path, struct stat *st);
int __wrap_stat(const char *path, struct stat *st) {
    MAPPED(path);
    return __real_stat(path, st);
}

int __real_access(const char *path, int mode);
int __wrap_access(const char *path, int mode) {
    MAPPED(path);
    return __real_access(path, mode);
}

int __real_open(const char *path, int flags, ...);
int __wrap_open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }
    MAPPED(path);
    return __real_open(path, flags, mode);
}

static bool RelativePathValid(const char *path) {
    if (!path || !*path || *path == '/' || strchr(path, ':') || strchr(path, '\\')) {
        return false;
    }
    const char *p = path;
    while (*p) {
        const char *end = strchr(p, '/');
        const size_t size = end ? (size_t)(end - p) : strlen(p);
        if (size == 0 || (size == 1 && p[0] == '.') || (size == 2 && p[0] == '.' && p[1] == '.')) {
            return false;
        }
        if (!end) {
            break;
        }
        p = end + 1;
    }
    return true;
}

static FILE *Open(const char *base, const char *path, const char *mode) {
    char full[768];
    if (!RelativePathValid(path) || !mode) {
        errno = EINVAL;
        CtrLog_Write(CTR_LOG_ERROR, "invalid filesystem path/mode");
        return NULL;
    }
    if (snprintf(full, sizeof(full), "%s%s", base, path) >= (int)sizeof(full)) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    FILE *file = fopen(full, mode);
    if (!file) {
        CtrLog_Write(CTR_LOG_ERROR, "open %s: errno=%d", full, errno);
    }
    return file;
}

bool CtrFs_Init(void) {
    __real_mkdir("sdmc:/switch", 0777);
    __real_mkdir(APP_DIR, 0777);
    __real_mkdir(APP_DIR "/data", 0777);
    mkdir("sdmc:/3ds/emerald3ds", 0777);

    FILE *file = CtrFs_OpenAsset("boot.txt");
    if (!file) {
        CtrLog_Write(CTR_LOG_ERROR, "data/boot.txt missing under " APP_DIR "/data");
        return true; /* CtrData_Init explains what is missing */
    }
    char contents[64] = {0};
    size_t n = fread(contents, 1, sizeof(contents) - 1, file);
    fclose(file);
    while (n > 0 && (contents[n - 1] == '\n' || contents[n - 1] == '\r')) {
        contents[--n] = 0;
    }
    CtrLog_Write(CTR_LOG_FS, "data boot.txt: %s", contents);
    return true;
}

void CtrFs_Shutdown(void) {}

FILE *CtrFs_OpenAsset(const char *relativePath) {
    return Open(ROMFS_PREFIX, relativePath, "rb");
}

FILE *CtrFs_OpenData(const char *relativePath, const char *mode) {
    return Open(SD_OLD_PREFIX "/", relativePath, mode);
}
