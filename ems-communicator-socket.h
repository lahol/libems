#pragma once

#include "ems-communicator.h"
#include "ems-util-list.h"
#include <stdarg.h>
#include <pthread.h>

/* This is the base class for communicators using some sort of socket for
 * communication, providing all the basic functionality apart from
 * establishing a connection and accepting an incoming connection.
 * EMSCommunicatorUnix and EMSCommunicatorInet are the main targets using
 * this class.
 *
 * The corresponding creator functions should call init first, do their
 * specific initialization routine, set try_connect and accept,
 * and call ems_communicator_socket_run_thread
 * afterwards.
 */
typedef enum {
    EMS_SOCKET_TYPE_UNKNOWN = 0,
    EMS_SOCKET_TYPE_CONTROL = 1,
    EMS_SOCKET_TYPE_MASTER,
    EMS_SOCKET_TYPE_DATA
} EMSSocketType;

typedef struct {
    int fd;
    EMSSocketType type;
    uint32_t id;
} EMSSocketInfo;

typedef struct _EMSCommunicatorSocket EMSCommunicatorSocket;

typedef int (*EMSCommunicatorSocketTryConnect)(EMSCommunicatorSocket *);
typedef int (*EMSCommunicatorSocketAccept)(EMSCommunicatorSocket *, int);

struct _EMSCommunicatorSocket {
    EMSCommunicator parent;

    EMSCommunicatorSocketTryConnect try_connect;
    EMSCommunicatorSocketAccept accept;

    /* <private> */
    int socket_fd;

    int epoll_fd;
    int control_pipe[2];
    pthread_t comm_thread;
    EMSList *socket_list;

    uint32_t comm_socket_status; /* internal status */
};

int ems_communicator_socket_init(EMSCommunicatorSocket *comm);
void ems_communicator_socket_set_value(EMSCommunicatorSocket *comm, const char *key, const void *value);
void ems_communicator_socket_clear(EMSCommunicatorSocket *comm);
int ems_communicator_socket_run_thread(EMSCommunicatorSocket *comm);

EMSSocketInfo *ems_communicator_socket_add_socket(EMSCommunicatorSocket *comm, int sockfd, EMSSocketType type);
int ems_communicator_socket_send_message(EMSCommunicatorSocket *comm, EMSMessage *msg);

