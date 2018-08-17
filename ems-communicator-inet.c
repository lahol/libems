#include "ems-communicator-inet.h"
#include "ems-memory.h"
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include "ems-util.h"
#include "ems-peer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ems-error.h"

/* Return the ip address of the host given by hostname. */
in_addr_t _ems_get_ip_address(const char *hostname) {
    struct in_addr addr;
    struct hostent *host;

    addr.s_addr = inet_addr(hostname);
    if (addr.s_addr == INADDR_NONE) {
        host = gethostbyname(hostname);
        if (!host)
            return INADDR_NONE;
        addr = *(struct in_addr*)host->h_addr_list[0];
    }
    return addr.s_addr;
}

int ems_communicator_inet_destroy(EMSCommunicatorInet *comm)
{
    ems_communicator_socket_clear((EMSCommunicatorSocket *)comm);
    free(comm->hostname);

    ems_free(comm);
    return 0;
}

int ems_communicator_inet_try_connect(EMSCommunicatorInet *comm)
{
    struct sockaddr_in addr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(comm->port);

    if (((EMSCommunicator *)comm)->role == EMS_PEER_ROLE_MASTER) {
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
            close(sockfd);
            return -1;
        }

        listen(sockfd, 10);
    }
    else {
        addr.sin_addr.s_addr = _ems_get_ip_address(comm->hostname);

        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
            close(sockfd);
            return -1;
        }
    }

    return sockfd;
}

int ems_communicator_inet_accept(EMSCommunicatorInet *comm, int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    return accept(fd, (struct sockaddr *)&addr, &len);
}

EMSCommunicator *ems_communicator_inet_create(va_list args)
{
    char *key;
    void *val;

    EMSCommunicator *comm = ems_alloc(sizeof(EMSCommunicatorInet));
    memset(comm, 0, sizeof(EMSCommunicatorInet));

    comm->type = EMS_COMM_TYPE_INET;
    comm->role = EMS_PEER_ROLE_SLAVE;
    comm->destroy = (EMSCommunicatorDestroy)ems_communicator_inet_destroy;

    ((EMSCommunicatorSocket *)comm)->try_connect =
        (EMSCommunicatorSocketTryConnect)ems_communicator_inet_try_connect;
    ((EMSCommunicatorSocket *)comm)->accept      =
        (EMSCommunicatorSocketAccept)ems_communicator_inet_accept;

    if (ems_communicator_socket_init((EMSCommunicatorSocket *)comm) != EMS_OK) {
        ems_free(comm);
        return NULL;
    }

    EMSCommunicatorInet *ic = (EMSCommunicatorInet *)comm;

    while ((key = va_arg(args, char *)) != NULL) {
        val = va_arg(args, void *);
        if (!strcmp(key, "hostname")) {
            if (ic->hostname) {
                free(ic->hostname);
            }
            ic->hostname = val ? strdup((const char *)val) : NULL;
        }
        else if (!strcmp(key, "port")) {
            ic->port = (uint16_t)EMS_UTIL_POINTER_TO_INT(val);
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
