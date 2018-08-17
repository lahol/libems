/* Communicaton over a UNIX domain socket. */
#pragma once

#include "ems-communicator-socket.h"
#include <stdarg.h>

typedef struct {
    /* Base class. */
    EMSCommunicatorSocket parent;

    /* Path of the named fifo this connection is associated to. */
    char socket_name[256];
} EMSCommunicatorUnix;

/* Create and set up a new communicator. */
EMSCommunicator *ems_communicator_unix_create(va_list args);
