#pragma once

#include "ems-communicator-socket.h"
#include <stdarg.h>

typedef struct {
    EMSCommunicatorSocket parent;

    char socket_name[256];
} EMSCommunicatorUnix;

EMSCommunicator *ems_communicator_unix_create(va_list args);
