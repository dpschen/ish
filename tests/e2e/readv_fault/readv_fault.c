#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0)
        die("sysconf");

    char *region = mmap(NULL, (size_t) page * 2, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED)
        die("mmap");

    if (munmap(region + page, (size_t) page) != 0)
        die("munmap");

    char *partial = region + page - 2;
    memset(region, 0, (size_t) page);

    char *head = malloc(4);
    if (head == NULL)
        die("malloc");
    memset(head, 0, 4);

    struct iovec vecs[2] = {
        { .iov_base = head, .iov_len = 4 },
        { .iov_base = partial, .iov_len = 4 },
    };

    int fds[2];
    if (pipe(fds) != 0)
        die("pipe");

    const char payload[] = "abcdefgh";
    ssize_t payload_len = (ssize_t) sizeof(payload);
    if (write(fds[1], payload, (size_t) payload_len) != payload_len)
        die("write");
    close(fds[1]);

    errno = 0;
    ssize_t got = readv(fds[0], vecs, 2);
    int saved_errno = errno;
    close(fds[0]);

    printf("readv returned %zd\n", got);
    printf("errno after readv: %d (%s)\n", saved_errno, strerror(saved_errno));
    printf("head buffer: %c%c%c%c\n", head[0], head[1], head[2], head[3]);
    printf("partial buffer bytes: %02x %02x\n",
            (unsigned char) partial[0], (unsigned char) partial[1]);

    if (got != 4) {
        fprintf(stderr, "expected 4 bytes before fault, got %zd\n", got);
        return 1;
    }
    if (memcmp(head, "abcd", 4) != 0) {
        fprintf(stderr, "unexpected bytes in head buffer\n");
        return 1;
    }
    if (partial[0] != 'e' || partial[1] != 'f') {
        fprintf(stderr, "partial buffer missing expected prefix\n");
        return 1;
    }
    munmap(region, (size_t) page);
    free(head);
    return 0;
}
