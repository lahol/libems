#include "ems-util-fd.h"

ssize_t ems_util_write_full(int fd, uint8_t *buffer, size_t length)
{
    ssize_t rc;
    ssize_t bytes_written = 0;

    while (bytes_written < length) {
        rc = write(fd, &buffer[bytes_written], length - bytes_written);
        if (rc < 0)
            return -1;
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
            return rc;
        }
        bytes_read += rc;
    }

    return bytes_read;
}
