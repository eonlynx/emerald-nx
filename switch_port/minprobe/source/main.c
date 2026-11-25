/* Minimal loader probe: no console, no fsdev, no data. */
#include <switch.h>
#include <string.h>

int main(void) {
    FsFileSystem fs;
    FsFile file;
    const char *msg = "Emerald NX: minprobe main reached\n";
    const char *path = "/switch/emerald-nx/min.log";
    if (R_FAILED(fsOpenSdCardFileSystem(&fs))) {
        svcExitProcess();
    }
    fsFsCreateDirectory(&fs, "/switch");
    fsFsCreateDirectory(&fs, "/switch/emerald-nx");
    fsFsCreateFile(&fs, path, 0, 0);
    if (R_SUCCEEDED(fsFsOpenFile(&fs, path, FsOpenMode_Write | FsOpenMode_Append, &file))) {
        s64 size = 0;
        fsFileGetSize(&file, &size);
        fsFileWrite(&file, size, msg, strlen(msg), FsWriteOption_Flush);
        fsFileClose(&file);
    }
    fsFsClose(&fs);
    svcExitProcess();
    return 0;
}
