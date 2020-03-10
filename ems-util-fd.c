#include "ems-util-fd.h"
#include <stdio.h>
#include "ems-util.h"

#include <poll.h>

ssize_t ems_util_write_full(int fd, uint8_t *buffer, size_t length)
{
    ssize_t rc;
    ssize_t bytes_written = 0;

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = 0;
    pfd.revents = 0;
    poll(&pfd, 1, 0);

    if (ems_unlikely(pfd.revents & POLLHUP))
        return -1;

    while (bytes_written < length) {
        rc = write(fd, &buffer[bytes_written], length - bytes_written);
        if (rc <= 0) {
            fprintf(stderr, "write_full returned %zd (written %zd/%zu)\n", rc, bytes_written, length);
            return rc;
        }
        bytes_written += rc;
    }

    return bytes_written;
}

ssize_t ems_util_read_full(int fd, uint8_t *buffer, size_t length)
{
    ssize_t bytes_read = 0;
    ssize_t rc;

    while (bytes_read < length) {
        rc = read(fd, &buffer[bytes_read], length - bytes_read);
        if (rc <= 0) {
            fprintf(stderr, "read_full returned %zd (written %zd/%zu)\n", rc, bytes_read, length);
            return rc;
        }
        bytes_read += rc;
    }

    return bytes_read;
}
