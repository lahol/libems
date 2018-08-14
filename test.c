#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ems-peer.h"
#include "ems.h"


int main(int argc, char **argv)
{
    int j;

    pid_t pid;

    ems_init("EMSG");

    EMSPeer *peer = NULL;

    EMSPeerRole role = EMS_PEER_ROLE_MASTER;

    for (j = 0; j < 5; ++j) {
        pid = fork();
        if (pid != 0) {
            fprintf(stderr, "forked: %d\n", pid);
        }
        else {
            role = EMS_PEER_ROLE_SLAVE;
            break;
        }
    }

    peer = ems_peer_create(role);

    EMSCommunicator *comm = ems_communicator_create(EMS_COMM_TYPE_UNIX,
                                                    "socket", "/tmp/test-ems-sock",
                                                    "role", role,
                                                    NULL, NULL);
    ems_peer_add_communicator(peer, comm);
    ems_peer_connect(peer);

    if (peer->role == EMS_PEER_ROLE_MASTER)
        fprintf(stderr, "I am the master. (%d)\n", getpid());
    else
        fprintf(stderr, "I am a slave. (%d)\n", getpid());

    /* We have no work yet. So just sleep to get the connection working. */
    sleep(3);

    ems_peer_destroy(peer);

    ems_cleanup();

    return 0;
}
