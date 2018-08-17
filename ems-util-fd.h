/* Utility functions to write/read a full buffer. */
#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/* Write a buffer of given length to the file descriptor fd.
 * This returns after the full buffer has been written or an
 * error occurred.
 */
ssize_t ems_util_write_full(int fd, uint8_t *buffer, size_t length);

/* Read length bytes into buffer from the file descriptor fd.
 * This returns after the full amount has been read or an
 * error occurred.
 */
ssize_t ems_util_read_full(int fd, uint8_t *buffer, size_t length);
