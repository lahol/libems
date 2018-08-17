/* Communicator to handle connections over the internet. */
#pragma once

#include "ems-communicator-socket.h"
#include <stdarg.h>
#include <stdint.h>

typedef struct {
    /* The base class of this communicator. This is socket based. */
    EMSCommunicatorSocket parent;

    /* The hostname of the remote peer. For the master this is ignored. */
    char *hostname;

    /* The port for the connection. */
    uint16_t port;
} EMSCommunicatorInet;

/* Create the communicator. */
EMSCommunicator *ems_communicator_inet_create(va_list args);
