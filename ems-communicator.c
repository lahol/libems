#include "ems-communicator.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ems-memory.h"
#include "ems-message.h"
#include "ems-messages-internal.h"
#include "ems-communicator-unix.h"
/*#include "ems-communicator-tcp.h"*/

EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...)
{
    va_list args;
    EMSCommunicator *comm = NULL;
    va_start(args, type);
    switch (type) {
        case EMS_COMM_TYPE_UNIX:
            comm = ems_communicator_unix_create(args);
            break;
        case EMS_COMM_TYPE_TCP:
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
            offsets[parts++] = j+1;
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
        comm = ems_communicator_create(EMS_COMM_TYPE_TCP,
                                       "host", offsets[1],
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
        return comm->send_message(comm);
    }
    return -1;
}

void ems_communicator_handle_internal_message(EMSCommunicator *comm, EMSMessage *msg)
{
    switch (msg->type) {
        case __EMS_MESSAGE_TYPE_SET_ID:
            if (comm && comm->peer)
                ems_peer_set_id(comm->peer, ((EMSMessageIntSetId *)msg)->peer_id);
            break;
        default:
            if (comm && comm->handle_int_message)
                comm->handle_int_message(comm, msg);
    }
}
