#pragma once

#include "ems-communicator.h"
#include <stdarg.h>

typedef struct {
    EMSCommunicator parent;

    char socket_name[256];
    int socket_fd;
} EMSCommunicatorUnix;

EMSCommunicator *ems_communicator_unix_create(va_list args);
