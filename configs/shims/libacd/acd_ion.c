#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

/* DMA-BUF Heap ioctl definitions */
struct dma_heap_allocation_data {
    __u64 len;
    __u32 fd;
    __u32 fd_flags;
    __u64 heap_flags;
};
#define DMA_HEAP_IOC_MAGIC		'H'
#define DMA_HEAP_IOCTL_ALLOC	_IOWR(DMA_HEAP_IOC_MAGIC, 0, struct dma_heap_allocation_data)

static int dma_heap_fd = -1;

/* 
 * Wrapper for legacy libion.so's ion_open()
 */
int ion_open(void) {
    if (dma_heap_fd >= 0) {
        return dma_heap_fd;
    }
    
    // Try opening standard system DMA-BUF heap
    dma_heap_fd = open("/dev/dma_heap/system", O_RDWR | O_CLOEXEC);
    if (dma_heap_fd < 0) {
        // Fallback for QCOM specific system heap name
        dma_heap_fd = open("/dev/dma_heap/qcom,system", O_RDWR | O_CLOEXEC);
    }
    
    if (dma_heap_fd < 0) {
        fprintf(stderr, "libacd wrapper: failed to open dma_heap (%s)\n", strerror(errno));
        return -1;
    }
    
    return dma_heap_fd;
}

/* 
 * Wrapper for legacy libion.so's ion_alloc_fd()
 * Original signature: 
 * int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask, unsigned int flags, int *handle_fd);
 */
int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask, unsigned int flags, int *handle_fd) {
    if (fd < 0 || !handle_fd) {
        return -EINVAL;
    }

    struct dma_heap_allocation_data alloc_data = {
        .len = len,
        .fd = 0,
        .fd_flags = O_RDWR | O_CLOEXEC,
        .heap_flags = 0, // Ignore legacy ION flags, system heap handles it
    };

    int ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &alloc_data);
    if (ret < 0) {
        fprintf(stderr, "libacd wrapper: DMA_HEAP_IOCTL_ALLOC failed for len %zu: %s\n", len, strerror(errno));
        return ret;
    }

    *handle_fd = alloc_data.fd;
    return 0;
}

/* 
 * Wrapper for legacy libion.so's ion_close()
 */
int ion_close(int fd) {
    if (fd >= 0) {
        int ret = close(fd);
        if (fd == dma_heap_fd) {
            dma_heap_fd = -1;
        }
        return ret;
    }
    return -EINVAL;
}
