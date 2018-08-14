#pragma once

#include "ems-message.h"
#include "ems-msg-queue.h"

typedef enum {
    EMS_COMM_TYPE_UNIX = 1,
    EMS_COMM_TYPE_TCP,
} EMSCommunicatorType;

typedef enum {
    EMS_COMM_STATUS_UNKNOWN     = 0,
    EMS_COMM_STATUS_INITIALIZED = 1,
    EMS_COMM_STATUS_CONNECTED,
    EMS_COMM_STATUS_RUNNING,
} EMSCommunicatorStatus;

typedef struct _EMSCommunicator EMSCommunicator;

typedef int (*EMSCommunicatorConnect)(EMSCommunicator *);
typedef int (*EMSCommunicatorDisconnect)(EMSCommunicator *);
typedef int (*EMSCommunicatorSendMessage)(EMSCommunicator *);
typedef void (*EMSCommunicatorDestroy)(EMSCommunicator *);
typedef void (*EMSCommunicatorHandleInternalMessage)(EMSCommunicator *, EMSMessage *);

#include "ems-peer.h"

struct _EMSCommunicator {
    EMSCommunicatorType type;
    EMSPeerRole role;

    EMSPeer *peer;

    EMSCommunicatorDestroy destroy;
    EMSCommunicatorConnect connect;
    EMSCommunicatorDisconnect disconnect;
    EMSCommunicatorSendMessage send_message;
    EMSCommunicatorHandleInternalMessage handle_int_message;

    EMSCommunicatorStatus status;

    EMSMessageQueue msg_queue_outgoing;
    EMSMessageQueue msg_queue_incoming;
};

EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...);
EMSCommunicator *ems_communicator_create_from_string(const char *desc);

void ems_communicator_set_role(EMSCommunicator *comm, EMSPeerRole role);

void ems_communicator_destroy(EMSCommunicator *comm);

int ems_communicator_connect(EMSCommunicator *comm);
int ems_communicator_disconnect(EMSCommunicator *comm);
int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg);
void ems_communicator_handle_internal_message(EMSCommunicator *comm, EMSMessage *msg);
