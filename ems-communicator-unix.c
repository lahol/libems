#include "ems-communicator-unix.h"
#include "ems-memory.h"
#include <memory.h>
#include <stdio.h>
#include <string.h>

int ems_communicator_unix_connect(EMSCommunicatorUnix *comm)
{
    return 0;
}

int ems_communicator_unix_send_message(EMSCommunicatorUnix *comm, EMSMessage *msg)
{
    return 0;
}

int ems_communicator_unix_run(EMSCommunicatorUnix *comm)
{
    return 0;
}

int ems_communicator_unix_stop(EMSCommunicatorUnix *comm)
{
    return 0;
}

int ems_communicator_unix_destroy(EMSCommunicatorUnix *comm)
{
    return 0;
}

EMSCommunicator *ems_communicator_unix_create(va_list args)
{
    char *key;
    void *val;

    EMSCommunicator *comm = ems_alloc(sizeof(EMSCommunicatorUnix));
    memset(comm, 0, sizeof(EMSCommunicatorUnix));

    comm->type = EMS_COMM_TYPE_UNIX;
    comm->destroy      = (EMSCommunicatorDestroy)ems_communicator_unix_destroy;
    comm->connect      = (EMSCommunicatorConnect)ems_communicator_unix_connect;
    comm->run          = (EMSCommunicatorRun)ems_communicator_unix_run;
    comm->send_message = (EMSCommunicatorSendMessage)ems_communicator_unix_send_message;
    comm->stop         = (EMSCommunicatorStop)ems_communicator_unix_stop;

    EMSCommunicatorUnix *uc = (EMSCommunicatorUnix *)comm;

    uc->socket_fd = -1;

    do {
        key = va_arg(args, char *);
        val = va_arg(args, void *);
        if (key) {
            if (!strcmp(key, "socket")) {
                strcpy(uc->socket_name, (char *)val);
            }
            else {
                fprintf(stderr, "unknown key: %s\n", key);
            }
        }
    } while (key);

    return uc;
}

