#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ems-peer.h"


int main(int argc, char **argv)
{
    int j;

    pid_t pid;

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
                                                    NULL, NULL);
    ems_peer_add_communicator(peer, comm);

    if (peer->role == EMS_PEER_ROLE_MASTER)
        fprintf(stderr, "I am the master.\n");
    else
        fprintf(stderr, "I am a slave.\n");

    ems_peer_destroy(peer);

    return 0;
}
