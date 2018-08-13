#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

ssize_t ems_util_write_full(int fd, uint8_t *buffer, size_t length);
ssize_t ems_util_read_full(int fd, uint8_t *buffer, size_t length);
