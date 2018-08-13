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

typedef uint32_t (*EMSCommunicatorQuerySlaveId)(EMSCommunicator *, void *);

typedef struct {
    EMSCommunicatorQuerySlaveId query_slave_id;
    /* handle message */
    void *user_data;
} EMSCommunicatorCallbacks;

struct _EMSCommunicator {
    EMSCommunicatorType type;
    int role;

    EMSCommunicatorDestroy destroy;
    EMSCommunicatorConnect connect;
    EMSCommunicatorDisconnect disconnect;
    EMSCommunicatorSendMessage send_message;

    EMSCommunicatorCallbacks callbacks;

    EMSCommunicatorStatus status;

    EMSMessageQueue msg_queue;
};

EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...);
void ems_communicator_destroy(EMSCommunicator *comm);

int ems_communicator_connect(EMSCommunicator *comm);
int ems_communicator_disconnect(EMSCommunicator *comm);
int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg);

void ems_communicator_set_callbacks(EMSCommunicator *comm, EMSCommunicatorCallbacks *callbacks);
