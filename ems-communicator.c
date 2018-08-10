#include "ems-communicator.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ems-memory.h"
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

    return comm;
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

int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg)
{
    if (comm && comm->send_message)
        return comm->send_message(comm, msg);
    return -1;
}

int ems_communicator_run(EMSCommunicator *comm)
{
    if (comm && comm->run)
        return comm->run(comm);
    return -1;
}

int ems_communicator_stop(EMSCommunicator *comm)
{
    if (comm && comm->stop)
        return comm->stop(comm);
    return -1;
}
