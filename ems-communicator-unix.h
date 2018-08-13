#pragma once

#include "ems-communicator.h"
#include "ems-util-list.h"
#include <stdarg.h>
#include <pthread.h>

typedef enum {
    EMS_COMM_UNIX_STATUS_CONNECTED  = (1<<0),  /* The socket is connected */
    EMS_COMM_UNIX_STATUS_CONNECTING = (1<<1),  /* The socket is about to be connected, but may have failed */
    EMS_COMM_UNIX_STATUS_DATA       = (1<<2),  /* Data is available */
    EMS_COMM_UNIX_STATUS_DISCONNECT = (1<<3),  /* Intent to disconnect */
    EMS_COMM_UNIX_STATUS_QUIT       = (1<<4 | 1<<3), /* Intent to quit */
} EMSCommunicatorUnixStatusFlag;

typedef struct {
    EMSCommunicator parent;

    char socket_name[256];
    int socket_fd;

    int epoll_fd;
    int control_pipe[2];
    pthread_t comm_thread;
    EMSList *socket_list;

    uint32_t status_flags;
} EMSCommunicatorUnix;

EMSCommunicator *ems_communicator_unix_create(va_list args);
