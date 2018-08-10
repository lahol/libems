#pragma once

#include "ems-message.h"

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

/*typedef int (*EMSCommunicatorInit)(EMSCommunicator *);*/
typedef int (*EMSCommunicatorConnect)(EMSCommunicator *);
typedef int (*EMSCommunicatorSendMessage)(EMSCommunicator *, EMSMessage *);
typedef int (*EMSCommunicatorRun)(EMSCommunicator *);
typedef int (*EMSCommunicatorStop)(EMSCommunicator *);
typedef void (*EMSCommunicatorDestroy)(EMSCommunicator *);

struct _EMSCommunicator {
    EMSCommunicatorType type;

/*    EMSCommunicatorInit init;*/
    EMSCommunicatorDestroy destroy;
    EMSCommunicatorConnect connect;
    EMSCommunicatorRun run;
    EMSCommunicatorStop stop;
    EMSCommunicatorSendMessage send_message;

    EMSCommunicatorStatus status;
};

EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...);
void ems_communicator_destroy(EMSCommunicator *comm);

int ems_communicator_connect(EMSCommunicator *comm);
int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg);
int ems_communicator_run(EMSCommunicator *comm);
int ems_communicator_stop(EMSCommunicator *comm);
