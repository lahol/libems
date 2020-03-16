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

#pragma once

#include "ems-communicator.h"
#include "ems-util-list.h"
#include <stdarg.h>
#include <pthread.h>

/* The different types of sockets to distinguish the actions to be taken when
 * the underlying file descriptor has data to read.
 */
typedef enum {
    /* Unknown type. */
    EMS_SOCKET_TYPE_UNKNOWN = 0,

    /* This is the control pipe used internally. */
    EMS_SOCKET_TYPE_CONTROL = 1,

    /* This is the master socket, responsible for accepting new connections. */
    EMS_SOCKET_TYPE_MASTER,

    /* This is a data socket for communication between the peers. */
    EMS_SOCKET_TYPE_DATA
} EMSSocketType;

/* Information about a socket/file descriptor. */
typedef struct {
    /* The file descriptor. */
    int fd;
    
    /* The type of this socket/fd. */
    EMSSocketType type;

    /* The id of the remote peer if this is a data socket. */
    uint64_t id;
} EMSSocketInfo;

typedef struct _EMSCommunicatorSocket EMSCommunicatorSocket;

/* The socket based communicators have different means of setting up a connection.
 * Try to connect the communicator. If successful, return EMS_OK, otherwise
 * return some error code and we will try again later.
 */
typedef int (*EMSCommunicatorSocketTryConnect)(EMSCommunicatorSocket *);

/* Accept an incoming connection. Only used if the peer is the master.
 * The second argument is the master file descriptor.
 */
typedef int (*EMSCommunicatorSocketAccept)(EMSCommunicatorSocket *, int);

/* Base class of socket based communicators. */
struct _EMSCommunicatorSocket {
    /* Base class of all communicators. */
    EMSCommunicator parent;

    /* To be filled by the realization of this communicator. */
    EMSCommunicatorSocketTryConnect try_connect;
    EMSCommunicatorSocketAccept accept;

    /* <private> */

    /* Our own file descriptor. This can be a connection to the master or the
     * accepting socket. */
    int socket_fd;

    /* File descriptor for epoll. */
    int epoll_fd;

    /* The control pipe for internal communication. */
    int control_pipe[2];

    /* The thread waiting for something to happen with the descriptors. */
    pthread_t comm_thread;

    /* List of file descriptors monitored by epoll. */
    EMSList *socket_list;

    /* Internal status of the communicator. */
    uint32_t comm_socket_status;
};

/* Initialize the communicator. Set up values and functions common to all derived communicators. */
int ems_communicator_socket_init(EMSCommunicatorSocket *comm);

/* Set a value corresponding to some key. */
void ems_communicator_socket_set_value(EMSCommunicatorSocket *comm, const char *key, const void *value);

/* Clean up the socket. */
void ems_communicator_socket_clear(EMSCommunicatorSocket *comm);

/* Run the thread. Only used once when setting up the derived communicator, after having set
 * all specific data.
 */
int ems_communicator_socket_run_thread(EMSCommunicatorSocket *comm);

/* Send a message over this communicator to all matching peers. */
int ems_communicator_socket_send_message(EMSCommunicatorSocket *comm, EMSMessage *msg);
