// Exercise the kernel's chunked sys_read implementation against a user buffer that
// faults partway through. The backing file must continue at the first unread byte
// so that callers can retry, so we assert the descriptor's position matches the
// bytes successfully copied out before the fault.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int main(void) {
    long page_size_raw = sysconf(_SC_PAGESIZE);
    if (page_size_raw <= 0) {
        fprintf(stderr, "sysconf(_SC_PAGESIZE) failed\n");
        return 1;
    }
    size_t page_size = (size_t) page_size_raw;

    size_t length = page_size * 2;
    // Map two pages and leave the tail inaccessible so read(2) will fault while
    // the kernel copies data back to userspace.
    char *buf = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (mprotect(buf + page_size, page_size, PROT_NONE) < 0) {
        perror("mprotect");
        munmap(buf, length);
        return 1;
    }

    const char *path = "/bin/busybox";
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        munmap(buf, length);
        return 1;
    }

    // The first chunk should copy successfully, after which the second chunk
    // triggers a user fault and the kernel must rewind the descriptor.
    ssize_t result = read(fd, buf, length);
    int saved_errno = errno;
    if (result != (ssize_t) page_size) {
        fprintf(stderr, "read returned %zd (errno %d), expected %zu\n", result, saved_errno, page_size);
        munmap(buf, length);
        return 1;
    }

    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos < 0) {
        perror("lseek");
        close(fd);
        munmap(buf, length);
        return 1;
    }
    if (pos != result) {
        fprintf(stderr, "lseek reported %lld after read, expected %lld\n", (long long) pos, (long long) result);
        close(fd);
        munmap(buf, length);
        return 1;
    }

    printf("OK: read returned %zu bytes and left file offset at %lld\n", page_size, (long long) pos);

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek reset");
        close(fd);
        munmap(buf, length);
        return 1;
    }

    // Mirror the same scenario through readv(2) so vectored paths rewind on a
    // fault just like the scalar syscall.
    struct iovec vecs[2] = {
        {.iov_base = buf, .iov_len = page_size},
        {.iov_base = buf + page_size, .iov_len = page_size},
    };
    ssize_t vres = readv(fd, vecs, 2);
    saved_errno = errno;
    if (vres != (ssize_t) page_size) {
        fprintf(stderr, "readv returned %zd (errno %d), expected %zu\n", vres, saved_errno, page_size);
        close(fd);
        munmap(buf, length);
        return 1;
    }

    off_t vpos = lseek(fd, 0, SEEK_CUR);
    if (vpos < 0) {
        perror("lseek after readv");
        close(fd);
        munmap(buf, length);
        return 1;
    }
    if (vpos != vres) {
        fprintf(stderr, "lseek reported %lld after readv, expected %lld\n", (long long) vpos, (long long) vres);
        close(fd);
        munmap(buf, length);
        return 1;
    }

    printf("OK: readv returned %zu bytes and left file offset at %lld\n", page_size, (long long) vpos);

    if (close(fd) < 0)
        perror("close");

    // Reading our own memory through /proc/self/mem used to crash because the
    // procfs seek helper always dereferenced ->show(). Make sure a simple
    // scalar read succeeds now that purely positional entries skip that path.
    int memfd = open("/proc/self/mem", O_RDONLY);
    if (memfd < 0) {
        perror("/proc/self/mem");
        munmap(buf, length);
        return 1;
    }

    off_t target = (off_t) (uintptr_t) buf;
    if (lseek(memfd, target, SEEK_SET) < 0) {
        perror("lseek /proc/self/mem");
        close(memfd);
        munmap(buf, length);
        return 1;
    }

    char scratch = 0;
    ssize_t mres = read(memfd, &scratch, 1);
    if (mres != 1) {
        fprintf(stderr, "read(/proc/self/mem) returned %zd\n", mres);
        close(memfd);
        munmap(buf, length);
        return 1;
    }

    if (close(memfd) < 0)
        perror("close /proc/self/mem");

    printf("OK: /proc/self/mem read succeeded without crashing\n");

    munmap(buf, length);
    return 0;
}
