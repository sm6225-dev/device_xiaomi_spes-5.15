#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <stddef.h>
#include <android/log.h>
#include <errno.h>

#define LOG_TAG "CamX-mmap-hook"
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    static void *(*real_mmap)(void *, size_t, int, int, int, off_t) = NULL;
    if (!real_mmap) {
        real_mmap = (void *(*)(void *, size_t, int, int, int, off_t))dlsym(RTLD_NEXT, "mmap");
        if (!real_mmap) real_mmap = (void *(*)(void *, size_t, int, int, int, off_t))dlsym(RTLD_DEFAULT, "mmap");
    }
    
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            // Check if the requested length is larger than the actual file size (e.g. dma-buf)
            if (st.st_size > 0 && length > (size_t)st.st_size) {
                ALOGW("mmap size mismatch intercepted! Requested length: %zu, FD st_size: %lld. Clamping to %lld to prevent EINVAL.", length, (long long)st.st_size, (long long)st.st_size);
                length = (size_t)st.st_size;
            }
        }
    }
    
    if (real_mmap) {
        return real_mmap(addr, length, prot, flags, fd, offset);
    }
    return MAP_FAILED;
}

void *mmap64(void *addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    static void *(*real_mmap64)(void *, size_t, int, int, int, off64_t) = NULL;
    if (!real_mmap64) {
        real_mmap64 = (void *(*)(void *, size_t, int, int, int, off64_t))dlsym(RTLD_NEXT, "mmap64");
        if (!real_mmap64) real_mmap64 = (void *(*)(void *, size_t, int, int, int, off64_t))dlsym(RTLD_DEFAULT, "mmap64");
    }
    
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            if (st.st_size > 0 && length > (size_t)st.st_size) {
                ALOGW("mmap64 size mismatch intercepted! Requested length: %zu, FD st_size: %lld. Clamping to %lld to prevent EINVAL.", length, (long long)st.st_size, (long long)st.st_size);
                length = (size_t)st.st_size;
            }
        }
    }
    
    if (real_mmap64) {
        return real_mmap64(addr, length, prot, flags, fd, offset);
    }
    return MAP_FAILED;
}
