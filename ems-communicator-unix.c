#include "ems-communicator-unix.h"
#include "ems-memory.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "ems-util.h"
#include "ems-peer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "ems-messages-internal.h"

EMSSocketInfo *_ems_communicator_unix_add_socket(EMSCommunicatorUnix *comm, int fd, EMSSocketType type)
{
    EMSSocketInfo *sock_info = ems_alloc(sizeof(EMSSocketInfo));
    sock_info->fd = fd;
    sock_info->type = type;
    sock_info->id = 0;

    comm->socket_list = ems_list_prepend(comm->socket_list, sock_info);

    return sock_info;
}

int ems_communicator_unix_connect(EMSCommunicatorUnix *comm)
{
    write(comm->control_pipe[1], "C", 1);
    return 0;
}

int ems_communicator_unix_disconnect(EMSCommunicatorUnix *comm)
{
    write(comm->control_pipe[1], "D", 1);
    return 0;
}

int ems_communicator_unix_send_message(EMSCommunicatorUnix *comm, EMSMessage *msg)
{
    /* signal incoming message to comm thread */
    if (ems_unlikely(!msg))
        return -1;
    ems_message_queue_push_tail(&((EMSCommunicator *)comm)->msg_queue_outgoing, ems_message_dup(msg));
    write(comm->control_pipe[1], "M", 1);
    return 0;
}

int ems_communicator_unix_destroy(EMSCommunicatorUnix *comm)
{
    write(comm->control_pipe[1], "Q", 1);

    pthread_join(comm->comm_thread, NULL);
    close(comm->control_pipe[1]);
    close(comm->control_pipe[0]);

    ems_list_free_full(comm->socket_list, (EMSDestroyNotifyFunc)ems_free);
    ems_message_queue_clear(&((EMSCommunicator *)comm)->msg_queue_outgoing);
    ems_message_queue_clear(&((EMSCommunicator *)comm)->msg_queue_incoming);

    return 0;
}

EMSSocketInfo *_ems_communictator_unix_try_connect(EMSCommunicatorUnix *comm)
{
    /* Depending on role, bind socket or connect */
    fprintf(stderr, "try connect %d\n", getpid());
    struct sockaddr_un addr;
    int sockfd;

    EMSSocketInfo *sock_info = NULL;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        return NULL;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, comm->socket_name, 108);

    if (((EMSCommunicator *)comm)->role == EMS_PEER_ROLE_MASTER) {
        /* bind socket */
        unlink(comm->socket_name);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
            fprintf(stderr, "error binding socket\n");
            close(sockfd);
            return NULL;
        }

        listen(sockfd, 10);

        sock_info = _ems_communicator_unix_add_socket(comm, sockfd, EMS_SOCKET_TYPE_MASTER);
        fprintf(stderr, "connected master\n");
    }
    else {
        /* try to connect to socket */
        if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0) {
            fprintf(stderr, "error connecting socket\n");
            close(sockfd);
            return NULL;
        }

        sock_info = _ems_communicator_unix_add_socket(comm, sockfd, EMS_SOCKET_TYPE_DATA);
        fprintf(stderr, "connected slave: %d\n", sockfd);
    }

    if (sock_info) {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = sock_info;
        epoll_ctl(comm->epoll_fd, EPOLL_CTL_ADD, sock_info->fd, &ev);
    }

    return sock_info;
}

int _ems_communicator_unix_master_accept_slave(EMSCommunicatorUnix *comm, int fd)
{
    fprintf(stderr, "try accepting slave\n");
    struct sockaddr_un addr;
    socklen_t len = sizeof(struct sockaddr_un);
    int newfd = accept(fd, (struct sockaddr *)&addr, &len);
    if (newfd < 0) {
        fprintf(stderr, "Error accepting.\n");
        return -1;
    }

    /* query new slave id and send to new slave */
    uint32_t new_id = 0;
    if (((EMSCommunicator *)comm)->peer)
        new_id = ems_peer_generate_new_slave_id(((EMSCommunicator *)comm)->peer);

    EMSSocketInfo *sock_info = _ems_communicator_unix_add_socket(comm, newfd, EMS_SOCKET_TYPE_DATA);
    sock_info->id = new_id;
    
    fprintf(stderr, "Accepted slave %d\n", new_id);
    /* set new client id */

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = sock_info;
    epoll_ctl(comm->epoll_fd, EPOLL_CTL_ADD, sock_info->fd, &ev);

    EMSMessage *msg = ems_message_new(__EMS_MESSAGE_TYPE_SET_ID,
                                      new_id,
                                      EMS_MESSAGE_RECIPIENT_MASTER,
                                      "peer-id", new_id,
                                      NULL, NULL);

    ems_communicator_unix_send_message(comm, msg);

    ems_message_free(msg);

    return 0;
}

void _ems_communicator_unix_disconnect_peers(EMSCommunicatorUnix *comm)
{
    /* remove everything not the control socket. */

    EMSList *tmp;
    EMSList *active = comm->socket_list;
    EMSSocketInfo *si;

    while (active) {
        tmp = active->next;
        si = (EMSSocketInfo *)active->data;

        if (si->type == EMS_SOCKET_TYPE_DATA || si->type == EMS_SOCKET_TYPE_MASTER) {
            epoll_ctl(comm->epoll_fd, EPOLL_CTL_DEL, si->fd, NULL);
            close(si->fd);
            ems_free(si);
            comm->socket_list = ems_list_delete_link(comm->socket_list, active);
        }
        active = tmp;
    }
}

void _ems_communicator_unix_disconnect_peer(EMSCommunicatorUnix *comm, EMSSocketInfo *sock_info)
{
    epoll_ctl(comm->epoll_fd, EPOLL_CTL_DEL, sock_info->fd, NULL);
    close(sock_info->fd);

    EMSList *tmp;
    for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
        if (tmp->data == sock_info) {
            ems_free(sock_info);
            comm->socket_list = ems_list_delete_link(comm->socket_list, tmp);
            break;
        }
    }
}

EMSSocketInfo *_ems_communicator_unix_get_peer(EMSCommunicatorUnix *comm, uint32_t peer_id)
{
    EMSList *tmp;
    for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
        if (((EMSSocketInfo *)tmp->data)->id == peer_id)
            return (EMSSocketInfo *)tmp->data;
    }
    return NULL;
}

void _ems_communicator_unix_check_outgoing_messages(EMSCommunicatorUnix *comm)
{
    EMSMessage *msg;
    EMSSocketInfo *peer;
    uint8_t *buffer = NULL;
    size_t buflen;
    EMSList *tmp;

    while ((msg = ems_message_queue_pop_filtered(&((EMSCommunicator *)comm)->msg_queue_outgoing)) != NULL) {
        buflen = ems_message_encode(msg, &buffer);
        if (msg->recipient_id == EMS_MESSAGE_RECIPIENT_ALL) {
            /* send to all */
            for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
                peer = (EMSSocketInfo *)tmp->data;
                if (peer->type == EMS_SOCKET_TYPE_DATA) {
                    ems_util_write_full(peer->fd, buffer, buflen);
                }
            }
        }
        else {
            /* find slave */
            peer = _ems_communicator_unix_get_peer(comm, msg->recipient_id);
            ems_util_write_full(peer->fd, buffer, buflen);
        }
        ems_free(buffer);
        ems_message_free(msg);
    }
}

void _ems_communicator_unix_read_incoming_message(EMSCommunicatorUnix *comm, EMSSocketInfo *sock_info)
{
    uint8_t *buffer = ems_alloc(EMS_MESSAGE_HEADER_SIZE);

    ssize_t rc;
    if ((rc = ems_util_read_full(sock_info->fd, buffer, EMS_MESSAGE_HEADER_SIZE)) < 0) {
        ems_free(buffer);
        return;
    }


    size_t payload_size = 0;
    EMSMessage *msg = ems_message_decode_header(buffer, rc, &payload_size);
    if (!msg)
        return;

    ems_free(buffer);
    if (payload_size) {
        buffer = ems_alloc(payload_size);

        if ((rc = ems_util_read_full(sock_info->fd, buffer, payload_size)) < 0) {
            ems_message_free(msg);
            ems_free(buffer);
            return;
        }

        ems_message_decode_payload(msg, buffer, payload_size);
    }

    if (EMS_MESSAGE_IS_INTERNAL(msg)) {
        ems_communicator_handle_internal_message((EMSCommunicator *)comm, msg);
        ems_message_free(msg);
    }
    else {
        ems_peer_push_message(((EMSCommunicator *)comm)->peer, msg);
    }
}

void *ems_communicator_unix_comm_thread(EMSCommunicatorUnix *comm)
{
    int epoll_timeout = -1;
    comm->epoll_fd = epoll_create1(0);

    EMSSocketInfo *sock_info;

    struct epoll_event ev;
    struct epoll_event incoming[16];
    int event_count, j;
    
    char ctl;
    int rc;

    ev.data.ptr = _ems_communicator_unix_add_socket(comm, comm->control_pipe[0], EMS_SOCKET_TYPE_CONTROL);
    ev.events = EPOLLIN;
    epoll_ctl(comm->epoll_fd, EPOLL_CTL_ADD, comm->control_pipe[0], &ev);

    while (1) {
        event_count = epoll_wait(comm->epoll_fd, incoming, 16, epoll_timeout);
        /* Handle messages */
        for (j = 0; j < event_count; ++j) {
            sock_info = (EMSSocketInfo *)incoming[j].data.ptr;
            switch (sock_info->type) {
                case EMS_SOCKET_TYPE_CONTROL:
                    {
                        /* handle control stuff */
                        if ((rc = read(sock_info->fd, &ctl, 1)) <= 0)
                            break;
                        if (ctl == 'M') {
                            /* New message arrived. Nothing to do here. This is just a wakeup call. */
                        }
                        else if (ctl == 'C') {
                            /* try to connect, if this fails, set timeout to 100ms, to retry */
                            comm->status_flags |= EMS_COMM_UNIX_STATUS_CONNECTING;
                        }
                        else if (ctl == 'D') {
                            /* disconnect, remove all data sockets, unbind if master */
                            comm->status_flags &= ~(EMS_COMM_UNIX_STATUS_CONNECTING | EMS_COMM_UNIX_STATUS_CONNECTED);
                            comm->status_flags |= EMS_COMM_UNIX_STATUS_DISCONNECT;
                        }
                        else if (ctl == 'Q') {
                            /* quit */
                            comm->status_flags &= ~(EMS_COMM_UNIX_STATUS_CONNECTING |
                                                    EMS_COMM_UNIX_STATUS_CONNECTED |
                                                    EMS_COMM_UNIX_STATUS_DATA);
                            comm->status_flags |= EMS_COMM_UNIX_STATUS_QUIT;
                        }
                    }
                    break;
                case EMS_SOCKET_TYPE_DATA:
                    /* read messages */
                    if (incoming[j].events & (EPOLLERR | EPOLLHUP)) {
/*                        fprintf(stderr, "Error in connection, removing\n");*/
                        _ems_communicator_unix_disconnect_peer(comm, sock_info);
                        /* FIXME: if we got no bye message and we are a slave, try to reconnect */
                    }
                    else if (incoming[j].events & EPOLLIN) {
                        _ems_communicator_unix_read_incoming_message(comm, sock_info);
                    }
                    break;
                case EMS_SOCKET_TYPE_MASTER:
                    /* accept incoming connections */
                    _ems_communicator_unix_master_accept_slave(comm, sock_info->fd);
                    break;
                default:
                    break;
            }
        }

        if (comm->status_flags & EMS_COMM_UNIX_STATUS_CONNECTING) {
            if ((sock_info = _ems_communictator_unix_try_connect(comm)) != NULL) {
                comm->status_flags &= ~EMS_COMM_UNIX_STATUS_CONNECTING;
                comm->status_flags |= EMS_COMM_UNIX_STATUS_CONNECTED;
                ((EMSCommunicator *)comm)->status = EMS_COMM_STATUS_CONNECTED;
                epoll_timeout = -1;
            }
            else
                epoll_timeout = 100;
        }
        else if (comm->status_flags & EMS_COMM_UNIX_STATUS_DISCONNECT) {
            _ems_communicator_unix_disconnect_peers(comm);
            comm->status_flags &= ~EMS_COMM_UNIX_STATUS_DISCONNECT;
            ((EMSCommunicator *)comm)->status = EMS_COMM_STATUS_INITIALIZED;
        }

        if (comm->status_flags & EMS_COMM_UNIX_STATUS_QUIT) {
            /* we disconnected earlier */
            fprintf(stderr, "quitting %d\n", getpid());
            break;
        }

        /* peek queue */
        _ems_communicator_unix_check_outgoing_messages(comm);
    }

    close(comm->epoll_fd);
    return NULL;
}

EMSCommunicator *ems_communicator_unix_create(va_list args)
{
    char *key;
    void *val;
    int rc;

    EMSCommunicator *comm = ems_alloc(sizeof(EMSCommunicatorUnix));
    memset(comm, 0, sizeof(EMSCommunicatorUnix));

    comm->role = EMS_PEER_ROLE_SLAVE; /* default to Slave if not specified otherwise */

    comm->type = EMS_COMM_TYPE_UNIX;
    comm->destroy      = (EMSCommunicatorDestroy)ems_communicator_unix_destroy;
    comm->connect      = (EMSCommunicatorConnect)ems_communicator_unix_connect;
    comm->disconnect   = (EMSCommunicatorDisconnect)ems_communicator_unix_disconnect;
    comm->send_message = (EMSCommunicatorSendMessage)ems_communicator_unix_send_message;

    EMSCommunicatorUnix *uc = (EMSCommunicatorUnix *)comm;

    uc->socket_fd = -1;

    do {
        key = va_arg(args, char *);
        val = va_arg(args, void *);
        if (key) {
            if (!strcmp(key, "socket")) {
                strncpy(uc->socket_name, (char *)val, 108);
            }
            else if (!strcmp(key, "role")) {
                comm->role = EMS_UTIL_POINTER_TO_INT(val);
            }
            else {
                fprintf(stderr, "unknown key: %s\n", key);
            }
        }
    } while (key);

    /* Set up control pipe */
    if (pipe(uc->control_pipe) != 0) {
        fprintf(stderr, "Could not set up control pipe.\n");
        ems_free(comm);
        return NULL;
    }

    ems_message_queue_init(&comm->msg_queue_outgoing);
    ems_message_queue_init(&comm->msg_queue_incoming);

    rc = pthread_create(&uc->comm_thread, NULL, (PThreadCallback)ems_communicator_unix_comm_thread, (void *)comm);
    if (rc) {
        fprintf(stderr, "Could not create the communication thread.\n");
        ems_free(comm);
        return NULL;
    }

    fprintf(stderr, "Created Unix communicator for %d.\n", getpid());
    return comm;
}

