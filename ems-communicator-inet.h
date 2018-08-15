#pragma once

#include "ems-communicator-socket.h"
#include <stdarg.h>
#include <stdint.h>

typedef struct {
    EMSCommunicatorSocket parent;

    char *hostname;
    uint16_t port;
} EMSCommunicatorInet;

EMSCommunicator *ems_communicator_inet_create(va_list args);
