/* Wrapper for the class specific communicators. */
#include "ems-communicator.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ems-memory.h"
#include "ems-message.h"
#include "ems-messages-internal.h"
#include "ems-communicator-unix.h"
#include "ems-communicator-inet.h"
#include "ems-status-messages.h"
#include <string.h>

/* Create a new communicator of the given type with the given key/value pairs. */
EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...)
{
    va_list args;
    EMSCommunicator *comm = NULL;
    va_start(args, type);
    switch (type) {
        case EMS_COMM_TYPE_UNIX:
            comm = ems_communicator_unix_create(args);
            break;
        case EMS_COMM_TYPE_INET:
            comm = ems_communicator_inet_create(args);
            break;
        default:
            fprintf(stderr, "Unsupported communicator type: %d\n", type);
    }
    va_end(args);

    comm->status = EMS_COMM_STATUS_INITIALIZED;

    return comm;
}

/* Quick and dirty approach to parse communicators.
 * unix:<filehandle>
 * inet:<ip>:<port>
 */
EMSCommunicator *ems_communicator_create_from_string(const char *desc)
{
    char *splt = strdup(desc);
    char *offsets[3];
    offsets[0] = splt;
    int j;
    int parts = 1;
    for (j = 0; splt[j] != '\0'; ++j) {
        if (splt[j] == ':') {
            splt[j] = '\0';
            offsets[parts++] = &splt[j+1];
        }
    }

    EMSCommunicator *comm = NULL;
    if (!strncmp(offsets[0], "unix", 4)) {
        if (parts < 2)
            goto done;
        comm = ems_communicator_create(EMS_COMM_TYPE_UNIX,
                                       "socket", offsets[1],
                                       NULL, NULL);
    }
    else if (!strncmp(offsets[0], "inet", 4)) {
        if (parts < 3)
            goto done;
        comm = ems_communicator_create(EMS_COMM_TYPE_INET,
                                       "hostname", offsets[1],
                                       "port", offsets[2],
                                       NULL, NULL);
    }

done:
    free(splt);
    return comm;
}

void ems_communicator_set_role(EMSCommunicator *comm, EMSPeerRole role)
{
    /* We cannot change the role if there is already a connection. */
    if (comm->status != EMS_COMM_STATUS_CONNECTED)
        comm->role = role;
}

void ems_communicator_destroy(EMSCommunicator *comm)
{
    if (comm && comm->destroy)
        comm->destroy(comm);
    else
        ems_free(comm);
}

int ems_communicator_connect(EMSCommunicator *comm)
{
    if (comm && comm->connect)
        return comm->connect(comm);
    return -1;
}

int ems_communicator_disconnect(EMSCommunicator *comm)
{
    if (comm && comm->disconnect)
        return comm->disconnect(comm);
    return -1;
}

int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg)
{
    if (comm && comm->send_message) {
        return comm->send_message(comm, msg);
    }
    return -1;
}

void ems_communicator_close_connection(EMSCommunicator *comm, uint64_t peer_id)
{
    if (comm && comm->close_connection)
        comm->close_connection(comm, peer_id);
}

void ems_communicator_flush_outgoing_messages(EMSCommunicator *comm)
{
    if (comm && comm->flush_outgoing)
        comm->flush_outgoing(comm);
}

void ems_communicator_handle_internal_message(EMSCommunicator *comm, EMSMessage *msg)
{
    EMSMessage *pmsg = NULL;
    switch (msg->type) {
        case __EMS_MESSAGE_SET_ID:
            if (comm && comm->peer) {
                ems_peer_set_id(comm->peer, ((EMSMessageIntSetId *)msg)->peer_id);

                pmsg = ems_message_new(EMS_MESSAGE_STATUS_PEER_READY,
                                       comm->peer->id,
                                       comm->peer->id,
                                       "peer", comm->peer,
                                       NULL, NULL);
                ems_peer_push_message(comm->peer, pmsg);
            }
            break;
        case __EMS_MESSAGE_LEAVE:
            ems_communicator_close_connection(comm, msg->sender_id);
            break;
        case __EMS_MESSAGE_TERM:
            ems_message_ref(msg);
            ems_peer_push_message(comm->peer, msg);
            break;
        default:
            if (comm && comm->handle_int_message)
                comm->handle_int_message(comm, msg);
    }
}

void ems_communicator_set_status(EMSCommunicator *comm, EMSCommunicatorStatus status)
{
    if (ems_unlikely(!comm))
        return;

    EMSMessage *msg = NULL;

    if (comm->status != status) {
        comm->status = status;
        /* signal status change */
        switch (status) {
            case EMS_COMM_STATUS_INITIALIZED:
                break;
            case EMS_COMM_STATUS_CONNECTED:
                if (comm->peer->role == EMS_PEER_ROLE_MASTER) {
                    msg = ems_message_new(EMS_MESSAGE_STATUS_PEER_READY,
                                          comm->peer->id,
                                          comm->peer->id,
                                          "peer", comm->peer,
                                          NULL, NULL);
                    ems_peer_push_message(comm->peer, msg);
                }
                break;
            default:
                break;
        }
    }
}

void ems_communicator_set_peer_id(EMSCommunicator *comm, uint64_t peer_id)
{
    if (ems_unlikely(!comm))
        return;

    comm->peer_id = peer_id;
}

void ems_communicator_add_connection(EMSCommunicator *comm)
{
    if (ems_unlikely(!comm))
        return;

    ++comm->open_connection_count;

    EMSMessage *msg = ems_message_new(__EMS_MESSAGE_CONNECTION_ADD, comm->peer_id, comm->peer_id, NULL, NULL);
    ems_peer_push_message(comm->peer, msg);
}

void ems_communicator_remove_connection(EMSCommunicator *comm, uint64_t remote_id)
{
    if (ems_unlikely(!comm))
        return;

    --comm->open_connection_count;
    EMSMessage *msg = ems_message_new(__EMS_MESSAGE_CONNECTION_DEL, comm->peer_id, comm->peer_id,
                                      "remote-id", remote_id,
                                      NULL, NULL);
    ems_peer_push_message(comm->peer, msg);
}
