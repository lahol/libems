#include "ems-communicator-socket.h"
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
#include <sys/eventfd.h>
/*#include <sys/un.h>*/
#include "ems-messages-internal.h"
#include "ems-error.h"

#ifdef DEBUG
#include <stdio.h>
#endif

typedef enum {
    _EMS_COMM_SOCKET_STATUS_CONTROL_PIPE   = (1 << 0), /* The control pipe is set up */
    _EMS_COMM_SOCKET_STATUS_THREAD_RUNNING = (1 << 1), /* The thread is running */
    _EMS_COMM_SOCKET_ACTION_CONNECTED  = (1<<2),  /* The socket is connected */
    _EMS_COMM_SOCKET_ACTION_CONNECTING = (1<<3),  /* The socket is about to be connected, but may have failed */
    _EMS_COMM_SOCKET_ACTION_DISCONNECT = (1<<4),  /* Intent to disconnect */
    _EMS_COMM_SOCKET_ACTION_QUIT       = (1<<5 | 1<<4), /* Intent to quit */
} _EMSCommunicatorSocketStatusFlag;

static
EMSSocketInfo *ems_communicator_socket_add_socket(EMSCommunicatorSocket *comm, int sockfd, EMSSocketType type)
{
    if (ems_unlikely(!comm) || sockfd < 0)
        return NULL;

    EMSSocketInfo *sock_info = ems_alloc(sizeof(EMSSocketInfo));
    sock_info->fd = sockfd;
    sock_info->type = type;
    sock_info->id = 0;

    comm->socket_list = ems_list_prepend(comm->socket_list, sock_info);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = sock_info;
    epoll_ctl(comm->epoll_fd, EPOLL_CTL_ADD, sockfd, &ev);

    return sock_info;
}

static
int _ems_communicator_socket_signal_event(EMSCommunicatorSocket *comm)
{
    const uint64_t u = 1;
    if (write(comm->control_eventfd, &u, sizeof(uint64_t)) != sizeof(uint64_t))
        return EMS_ERROR_WRITE_FAILED;
    return EMS_OK;
}

static
int ems_communicator_socket_connect(EMSCommunicatorSocket *comm)
{
    atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_ACTION_CONNECTING);
    return _ems_communicator_socket_signal_event(comm);
}

static
int ems_communicator_socket_disconnect(EMSCommunicatorSocket *comm)
{
    atomic_fetch_and(&comm->comm_socket_status,
            ~(_EMS_COMM_SOCKET_ACTION_CONNECTING | _EMS_COMM_SOCKET_ACTION_CONNECTED));
    atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_ACTION_DISCONNECT);
    return _ems_communicator_socket_signal_event(comm);
}

int ems_communicator_socket_send_message(EMSCommunicatorSocket *comm, EMSMessage *msg)
{
    /* signal incoming message to comm thread */
    if (ems_unlikely(!msg))
        return EMS_ERROR_INVALID_ARGUMENT;
    ems_message_ref(msg);
    ems_message_queue_push_tail(&((EMSCommunicator *)comm)->msg_queue_outgoing, msg);
    return _ems_communicator_socket_signal_event(comm);
}

static
int ems_communicator_socket_try_connect(EMSCommunicatorSocket *comm)
{
    if (ems_unlikely(!comm || !comm->try_connect))
        return EMS_ERROR_INVALID_ARGUMENT;

    int sockfd = comm->try_connect(comm);
    if (sockfd < 0)
        return EMS_ERROR_CONNECTION;

    ems_communicator_socket_add_socket(comm, sockfd,
            ((EMSCommunicator *)comm)->role == EMS_PEER_ROLE_MASTER ? EMS_SOCKET_TYPE_MASTER : EMS_SOCKET_TYPE_DATA);

    if (((EMSCommunicator *)comm)->role == EMS_PEER_ROLE_SLAVE)
        ems_communicator_add_connection((EMSCommunicator *)comm);

    return EMS_OK;
}

static
int ems_communicator_socket_accept(EMSCommunicatorSocket *comm, int fd)
{
    if (ems_unlikely(!comm) || fd < 0)
        return EMS_ERROR_INVALID_ARGUMENT;
    if (ems_unlikely(!comm->accept))
        return EMS_ERROR_MISSING_CLASS_FUNCTION;

    int newfd = comm->accept(comm, fd);
    if (newfd < 0)
        return EMS_ERROR_CONNECTION;

    EMSSocketInfo *socket_info = ems_communicator_socket_add_socket(comm, newfd, EMS_SOCKET_TYPE_DATA);

    ems_communicator_add_connection((EMSCommunicator *)comm);

    uint64_t new_id = 0;
    new_id = ems_peer_generate_new_slave_id(((EMSCommunicator *)comm)->peer);
    socket_info->id = new_id;

    EMSMessage *msg = ems_message_new(__EMS_MESSAGE_SET_ID,
                                      new_id,
                                      EMS_MESSAGE_RECIPIENT_MASTER,
                                      "peer-id", new_id,
                                      NULL, NULL);

    ems_communicator_socket_send_message(comm, msg);

    ems_message_unref(msg);

    return EMS_OK;

}

/* Remove all sockets not being the control socket. */
static
void ems_communicator_socket_disconnect_peers(EMSCommunicatorSocket *comm)
{
    EMSList *tmp;
    EMSList *active = comm->socket_list;
    EMSSocketInfo *si;

    while (active) {
        tmp = active->next;
        si = (EMSSocketInfo *)active->data;

        if (si->type == EMS_SOCKET_TYPE_DATA) {
            ems_communicator_remove_connection((EMSCommunicator *)comm, si->id);
        }
        if (si->type == EMS_SOCKET_TYPE_DATA || si->type == EMS_SOCKET_TYPE_MASTER) {
            epoll_ctl(comm->epoll_fd, EPOLL_CTL_DEL, si->fd, NULL);
            close(si->fd);
            ems_free(si);
            comm->socket_list = ems_list_delete_link(comm->socket_list, active);
        }
        active = tmp;
    }
}

/* Remove the specified socket. */
static
void ems_communicator_socket_disconnect_peer(EMSCommunicatorSocket *comm, EMSSocketInfo *sock_info)
{
    epoll_ctl(comm->epoll_fd, EPOLL_CTL_DEL, sock_info->fd, NULL);
    close(sock_info->fd);

    EMSList *tmp;
    for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
        if (tmp->data == sock_info) {
            comm->socket_list = ems_list_delete_link(comm->socket_list, tmp);
            ems_communicator_remove_connection((EMSCommunicator *)comm, sock_info->id);
            ems_free(sock_info);
            break;
        }
    }
}

static
void ems_communicator_socket_close_connection(EMSCommunicatorSocket *comm, uint64_t peer_id)
{
    EMSList *tmp;
    EMSSocketInfo *sock_info = NULL;
    for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
        if (((EMSSocketInfo *)tmp->data)->id == peer_id) {
            sock_info = (EMSSocketInfo *)tmp->data;

            epoll_ctl(comm->epoll_fd, EPOLL_CTL_DEL, sock_info->fd, NULL);
            close(sock_info->fd);

            comm->socket_list = ems_list_delete_link(comm->socket_list, tmp);
            ems_communicator_remove_connection((EMSCommunicator *)comm, sock_info->id);
            ems_free(sock_info);
            break;
        }
    }
}

/* Find the socket associated to the given peer. */
static
EMSSocketInfo *_ems_communicator_socket_get_peer(EMSCommunicatorSocket *comm, uint64_t peer_id)
{
    EMSList *tmp;
    for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
        if (((EMSSocketInfo *)tmp->data)->id == peer_id)
            return (EMSSocketInfo *)tmp->data;
    }
    return NULL;
}

/* Check for outgoing messages and deliver them to the peers. */
static
void _ems_communicator_socket_check_outgoing_messages(EMSCommunicatorSocket *comm)
{
    EMSMessage *msg;
    EMSSocketInfo *peer;
    uint8_t *buffer = NULL;
    size_t buflen;
    EMSList *tmp;
    ssize_t rc;

    EMSList *err_list = NULL;

    while ((msg = ems_message_queue_pop_filtered(&((EMSCommunicator *)comm)->msg_queue_outgoing)) != NULL) {
        buflen = ems_message_encode(msg, &buffer);
        if (msg->recipient_id == EMS_MESSAGE_RECIPIENT_ALL) {
            /* send to all */
            for (tmp = comm->socket_list; tmp; tmp = tmp->next) {
                peer = (EMSSocketInfo *)tmp->data;
#ifdef DEBUG
                fprintf(stderr, "[%d] Send message 0x%08x to %" PRIu64 "\n", getpid(), msg->type, peer->id);
#endif
                if (peer->type == EMS_SOCKET_TYPE_DATA) {
                    if ((rc = ems_util_write_full(peer->fd, buffer, buflen)) <= 0) {
#ifdef DEBUG
                        fprintf(stderr, "[%d] write to %" PRIu64 " returned %ld\n", getpid(), peer->id, rc);
#endif
                        err_list = ems_list_prepend(err_list, tmp->data);
                    }
                }
            }
            /* remove hung up descriptors */
            while (ems_unlikely(err_list != NULL)) {
                tmp = err_list->next;
#ifdef DEBUG
                fprintf(stderr, "[%d] There have been descriptor errors: id=%u\n", getpid(), ((EMSSocketInfo *)err_list->data)->id);
#endif
                ems_communicator_socket_disconnect_peer(comm, (EMSSocketInfo *)err_list->data);
                ems_free(err_list);
                err_list = tmp;
            }
        }
        else {
            /* find slave */
            peer = _ems_communicator_socket_get_peer(comm, msg->recipient_id);
            if (peer && ems_util_write_full(peer->fd, buffer, buflen) < 0)
                ems_communicator_socket_disconnect_peer(comm, peer);
        }
        ems_free(buffer);
        ems_message_unref(msg);
    }
}

static
void ems_communicator_socket_flush_outgoing_messages(EMSCommunicatorSocket *comm)
{
    _ems_communicator_socket_check_outgoing_messages(comm);
}

/* Read an incoming message, decode it and push it to the message queue of the peer. */
static
int _ems_communicator_socket_read_incoming_message(EMSCommunicatorSocket *comm, EMSSocketInfo *sock_info)
{
    uint8_t *buffer = ems_alloc(EMS_MESSAGE_HEADER_SIZE);

    ssize_t rc;
    if ((rc = ems_util_read_full(sock_info->fd, buffer, EMS_MESSAGE_HEADER_SIZE)) <= 0) {
#ifdef DEBUG
        fprintf(stderr, "[%d] read_full returned %ld\n", getpid(), rc);
#endif
        ems_free(buffer);
        return EMS_ERROR_INVALID_SOCKET;
    }

    size_t payload_size = 0;
/*    fprintf(stderr, "[%d] msgheader: "
                    "%02x %02x %02x %02x %02x %02x %02x %02x "
                    "%02x %02x %02x %02x %02x %02x %02x %02x "
                    "%02x %02x %02x %02x\n", getpid(),
                    buffer[0], buffer[1], buffer[2], buffer[3],
                    buffer[4], buffer[5], buffer[6], buffer[7],
                    buffer[8], buffer[9], buffer[10], buffer[11],
                    buffer[12], buffer[13], buffer[14], buffer[15],
                    buffer[16], buffer[17], buffer[18], buffer[19]);*/

    EMSMessage *msg = ems_message_decode_header(buffer, rc, &payload_size);
    if (!msg)
        return EMS_ERROR_INVALID_ARGUMENT;

    ems_free(buffer);
    if (payload_size) {
        buffer = ems_alloc(payload_size);

        if ((rc = ems_util_read_full(sock_info->fd, buffer, payload_size)) <= 0) {
#ifdef DEBUG
            fprintf(stderr, "[%d] read_full returned %ld\n", getpid(), rc);
#endif
            ems_message_unref(msg);
            ems_free(buffer);
            return EMS_ERROR_INVALID_SOCKET;
        }

        ems_message_decode_payload(msg, buffer, payload_size);
        ems_free(buffer);
    }

    /* FIXME: Do we really need this distinction? Can’t we just push to the peer and
     * let the peer handle this? */
    if (EMS_MESSAGE_IS_INTERNAL(msg)) {
        ems_communicator_handle_internal_message((EMSCommunicator *)comm, msg);
        ems_message_unref(msg);
    }
    else {
        ems_peer_push_message(((EMSCommunicator *)comm)->peer, msg);
    }

    return EMS_OK;
}

/* The main thread of the communicator. Wait for data in the control socket, new data,
 * or incoming connections.
 */
static
void *ems_communicator_socket_comm_thread(EMSCommunicatorSocket *comm)
{
    int epoll_timeout = -1;
    comm->epoll_fd = epoll_create1(0);

    EMSSocketInfo *sock_info;

    struct epoll_event incoming[16];
    int event_count, j;

    int rc;

    ems_communicator_socket_add_socket(comm, comm->control_eventfd, EMS_SOCKET_TYPE_CONTROL);

    while (1) {
        event_count = epoll_wait(comm->epoll_fd, incoming, 16, epoll_timeout);
        /* Handle messages */
        for (j = 0; j < event_count; ++j) {
            sock_info = (EMSSocketInfo *)incoming[j].data.ptr;
            switch (sock_info->type) {
                case EMS_SOCKET_TYPE_CONTROL:
                    {
                        /* Just wake up and read from the eventfd. */
                        uint64_t u;
                        (void)read(sock_info->fd, &u, sizeof(uint64_t));
                    }
                    break;
                case EMS_SOCKET_TYPE_DATA:
                   /* read messages */
                    if (incoming[j].events & (EPOLLERR | EPOLLHUP)) {
#ifdef DEBUG
                        fprintf(stderr, "[%d] data available, events: 0x%02x\n", getpid(), incoming[j].events);
#endif
                         ems_communicator_socket_disconnect_peer(comm, sock_info);
                        /* FIXME: if we got no bye message and we are a slave, try to reconnect */
                    }
                    else if (incoming[j].events & EPOLLIN) {
                        rc = _ems_communicator_socket_read_incoming_message(comm, sock_info);
                        if (rc == EMS_ERROR_INVALID_SOCKET) {
                            ems_communicator_socket_disconnect_peer(comm, sock_info);
                        }
                    }
                    break;
                case EMS_SOCKET_TYPE_MASTER:
                    /* accept incoming connections */
                    ems_communicator_socket_accept(comm, sock_info->fd);
                    break;
                default:
                    break;
            }
        }

        /* The communicator should connect. If successful, disable the epoll timeout
         * and set the status (and flags) appropriately. If not, the timeout is set
         * to 100 ms to try again later.
         */
        if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_ACTION_CONNECTING) {
#ifdef DEBUG
            fprintf(stderr, "[%d] try connecting\n", getpid());
#endif
            if ((rc = ems_communicator_socket_try_connect(comm)) == EMS_OK) {
                atomic_fetch_and(&comm->comm_socket_status, ~_EMS_COMM_SOCKET_ACTION_CONNECTING);
                atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_ACTION_CONNECTED);
                ems_communicator_set_status((EMSCommunicator *)comm, EMS_COMM_STATUS_CONNECTED);
                epoll_timeout = -1;
            }
            else {
#ifdef DEBUG
                fprintf(stderr, "[%d] try connecting returned %d\n", getpid(), rc);
#endif
                epoll_timeout = 100;
            }
        }
        else if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_ACTION_DISCONNECT) {
            ems_communicator_socket_disconnect_peers(comm);
            atomic_fetch_and(&comm->comm_socket_status, ~_EMS_COMM_SOCKET_ACTION_DISCONNECT);
            ems_communicator_set_status((EMSCommunicator *)comm, EMS_COMM_STATUS_INITIALIZED);
        }

        if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_ACTION_QUIT) {
            /* we disconnected earlier */
            break;
        }

        /* peek queue, only if connected */
        if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_ACTION_CONNECTED)
            _ems_communicator_socket_check_outgoing_messages(comm);
    }

    close(comm->epoll_fd);

    return NULL;
}

int ems_communicator_socket_init(EMSCommunicatorSocket *comm)
{
    EMSCommunicator *c = (EMSCommunicator *)comm;
    c->connect      = (EMSCommunicatorConnect)ems_communicator_socket_connect;
    c->disconnect   = (EMSCommunicatorDisconnect)ems_communicator_socket_disconnect;
    c->send_message = (EMSCommunicatorSendMessage)ems_communicator_socket_send_message;
    c->close_connection = (EMSCommunicatorCloseConnection)ems_communicator_socket_close_connection;
    c->flush_outgoing = (EMSCommunicatorFlushOutgoingMessages)ems_communicator_socket_flush_outgoing_messages;

    if ((comm->control_eventfd = eventfd(0, 0)) == -1) {
        fprintf(stderr, "EMSCommunicatorSocket: Could not set up control fd.\n");
        return EMS_ERROR_INITIALIZATION;
    }

    atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_STATUS_CONTROL_PIPE);

    ems_message_queue_init(&((EMSCommunicator *)comm)->msg_queue_outgoing);
    ems_message_queue_init(&((EMSCommunicator *)comm)->msg_queue_incoming);

    return EMS_OK;
}

void ems_communicator_socket_set_value(EMSCommunicatorSocket *comm, const char *key, const void *value)
{
    if (ems_unlikely(!comm))
        return;
    if (!strcmp(key, "role")) {
        /* FIXME: set role -> possibly disconnect slaves and close listen socket (master->slave) */
        ((EMSCommunicator *)comm)->role = EMS_UTIL_POINTER_TO_INT(value);
    }
    else {
        fprintf(stderr, "EMSCommunicatorSocket: Unknown key: %s\n", key);
    }
}

void ems_communicator_socket_clear(EMSCommunicatorSocket *comm)
{
    if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_STATUS_CONTROL_PIPE) {
        atomic_fetch_and(&comm->comm_socket_status,
                ~(_EMS_COMM_SOCKET_ACTION_CONNECTING | _EMS_COMM_SOCKET_ACTION_CONNECTED));
        atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_ACTION_QUIT);
        const uint64_t u = 1;
        if (write(comm->control_eventfd, &u, sizeof(uint64_t)) != sizeof(uint64_t))
            fprintf(stderr, "EMSCommunicatorSocket: Could not write to control pipe, quit\n");
    }

    if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_STATUS_THREAD_RUNNING) {
        pthread_join(comm->comm_thread, NULL);
        atomic_fetch_and(&comm->comm_socket_status, ~_EMS_COMM_SOCKET_STATUS_THREAD_RUNNING);
    }

    if (atomic_load(&comm->comm_socket_status) & _EMS_COMM_SOCKET_STATUS_CONTROL_PIPE) {
        close(comm->control_eventfd);

        comm->control_eventfd = -1;

        atomic_fetch_and(&comm->comm_socket_status, ~_EMS_COMM_SOCKET_STATUS_CONTROL_PIPE);
    }

    ems_list_free_full(comm->socket_list, (EMSDestroyNotifyFunc)ems_free);
    comm->socket_list = NULL;

    ems_message_queue_clear(&((EMSCommunicator *)comm)->msg_queue_outgoing);
    ems_message_queue_clear(&((EMSCommunicator *)comm)->msg_queue_incoming);
}

int ems_communicator_socket_run_thread(EMSCommunicatorSocket *comm)
{
    int rc;
    if (ems_unlikely(!comm))
        return EMS_ERROR_INITIALIZATION;

    rc = pthread_create(&comm->comm_thread, NULL, (PThreadCallback)ems_communicator_socket_comm_thread, (void *)comm);
    if (rc) {
        fprintf(stderr, "EMSCommunicatorSocket: Could not create the communication thread.\n");
        return EMS_ERROR_INITIALIZATION;
    }

    atomic_fetch_or(&comm->comm_socket_status, _EMS_COMM_SOCKET_STATUS_THREAD_RUNNING);

    return EMS_OK;
}
