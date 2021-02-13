#include "ems-communicator-unix.h"
#include "ems-memory.h"
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include "ems-util.h"
#include "ems-peer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ems-error.h"

static
int ems_communicator_unix_destroy(EMSCommunicatorUnix *comm)
{
    ems_communicator_socket_clear((EMSCommunicatorSocket *)comm);
    ems_free(comm);
    return 0;
}

static
int ems_communictator_unix_try_connect(EMSCommunicatorUnix *comm)
{
    /* Depending on role, bind socket or connect */
    struct sockaddr_un addr;
    int sockfd;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, comm->socket_name, 108);

    if (((EMSCommunicator *)comm)->role == EMS_PEER_ROLE_MASTER) {
        /* bind socket */
        unlink(comm->socket_name);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
            close(sockfd);
            return -1;
        }

        listen(sockfd, 10);
    }
    else {
        /* try to connect to socket */
        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
            close(sockfd);
            return -1;
        }
    }

    return sockfd;
}

static
int ems_communicator_unix_accept(EMSCommunicatorUnix *comm, int fd)
{
    struct sockaddr_un addr;
    socklen_t len = sizeof(struct sockaddr_un);
    return accept(fd, (struct sockaddr *)&addr, &len);
}

EMSCommunicator *ems_communicator_unix_create(va_list args)
{
    char *key;
    void *val;

    EMSCommunicator *comm = ems_alloc(sizeof(EMSCommunicatorUnix));
    memset(comm, 0, sizeof(EMSCommunicatorUnix));

    comm->type = EMS_COMM_TYPE_UNIX;
    comm->role = EMS_PEER_ROLE_SLAVE; /* default to Slave if not specified otherwise */
    comm->destroy = (EMSCommunicatorDestroy)ems_communicator_unix_destroy;

    ((EMSCommunicatorSocket *)comm)->try_connect =
        (EMSCommunicatorSocketTryConnect)ems_communictator_unix_try_connect;
    ((EMSCommunicatorSocket *)comm)->accept      =
        (EMSCommunicatorSocketAccept)ems_communicator_unix_accept;

    if (ems_communicator_socket_init((EMSCommunicatorSocket *)comm) != EMS_OK) {
        ems_free(comm);
        return NULL;
    }

    EMSCommunicatorUnix *uc = (EMSCommunicatorUnix *)comm;

    while ((key = va_arg(args, char *)) != NULL) {
        val = va_arg(args, void *);
        if (!strcmp(key, "socket")) {
            strncpy(uc->socket_name, (char *)val, 107);
        }
        else {
            ems_communicator_socket_set_value((EMSCommunicatorSocket *)comm, key, val);
        }
    }

    if (ems_communicator_socket_run_thread((EMSCommunicatorSocket *)comm) != EMS_OK) {
        ems_communicator_socket_clear((EMSCommunicatorSocket *)comm);
        ems_free(comm);
        return NULL;
    }

    return comm;
}
