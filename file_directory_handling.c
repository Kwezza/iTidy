#include "file_directory_handling.h"

int HasSlaveFile(char *path) {
    BPTR lock;
    struct FileInfoBlock *fib;
    int hasSlave = 0;

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0) {
        Printf("Failed to lock directory: %s\n", path);
        return 0;
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib == NULL) {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return 0;
    }

    if (Examine(lock, fib)) {
        while (ExNext(lock, fib)) {
            if (fib->fib_DirEntryType < 0) {
                const char *ext = strrchr(fib->fib_FileName, '.');
                if (ext && strncasecmp_custom(ext, ".slave", 6) == 0) {
                    hasSlave = 1;
                    break;
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
    return hasSlave;
}

void ProcessDirectory(char *path, BOOL processSubDirs) {
    BPTR lock;
    struct FileInfoBlock *fib;
    char subdir[4096];

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (lock == 0) {
        Printf("Failed to lock directory: %s\n", path);
        return;
    }

    if (HasSlaveFile(path)) {
        resizeFolderToContents(path, CreateIconArrayFromPath(lock, path));
        UnLock(lock);
        return;
    }

    fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (fib == NULL) {
        Printf("Failed to allocate memory for FileInfoBlock\n");
        UnLock(lock);
        return;
    }

    if (Examine(lock, fib)) {
        FormatIconsAndWindow(path);
        if (processSubDirs == TRUE) {
            while (ExNext(lock, fib)) {
                if (fib->fib_DirEntryType > 0) {
                    sprintf(subdir, "%s/%s", path, fib->fib_FileName);
                    ProcessDirectory(subdir, TRUE);
                }
            }
        }
    }

    FreeMem(fib, sizeof(struct FileInfoBlock));
    UnLock(lock);
}

void GetFullPath(const char *directory, struct FileInfoBlock *fib, char *fullPath, int fullPathSize) {
    int dirLen;

    if (directory == NULL || fib == NULL || fullPath == NULL || fullPathSize <= 0) {
        return;
    }

    strncpy(fullPath, directory, fullPathSize - 1);
    fullPath[fullPathSize - 1] = '\0';

    dirLen = strlen(fullPath);
    if (dirLen > 0 && fullPath[dirLen - 1] != '/' && fullPath[dirLen - 1] != ':') {
        if (dirLen + 1 < fullPathSize) {
            strncat(fullPath, "/", fullPathSize - dirLen - 1);
            dirLen++;
        }
    }

    strncat(fullPath, fib->fib_FileName, fullPathSize - dirLen - 1);
    fullPath[fullPathSize - 1] = '\0';
}

int IsRootDirectorySimple(char *path) {
    size_t length = strlen(path);
    if (length > 0 && path[length - 1] == ':') {
        return 1;
    }
    return 0;
}
