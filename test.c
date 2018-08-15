#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ems-peer.h"
#include "ems.h"

#define EMS_TEST_MESSAGE_QUIT      1

void test_register_messages(void)
{
    EMSMessageClass cls;
    cls.msgtype = EMS_TEST_MESSAGE_QUIT;
    cls.size = sizeof(EMSMessage);
    cls.min_payload = 0;
    cls.msg_encode = NULL;
    cls.msg_decode = NULL;
    cls.msg_free = NULL;
    cls.msg_set_value = NULL;
    cls.msg_copy = NULL;

    ems_message_register_type(EMS_TEST_MESSAGE_QUIT, &cls);
}

#include <pthread.h>
#include <signal.h>

EMSPeerRole role = EMS_PEER_ROLE_MASTER;

#if 0
static void *signal_thread(void *arg)
{
    sigset_t *set = arg;
    int s, sig;

    while (1) {
        s = sigwait(set, &sig);
        if (s != 0) {
            fprintf(stderr, "Error in sigwait\n");
        }
/*        fprintf(stderr, "Signal handling thread got signal %d\n", sig);*/

        if (role == EMS_PEER_ROLE_MASTER && sig == SIGINT) {
            fprintf(stderr, "Master got SIGINT\n");
        }
    }

    return NULL;
}
#endif

int main(int argc, char **argv)
{
    int j;

    pid_t pid;

    ems_init("EMSG");

    EMSPeer *peer = NULL;

    EMSMessage *msg;

    test_register_messages();

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

#if 0
    /* signal handling */
    pthread_t sig_thread;
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);

    pthread_sigmask(SIG_BLOCK, &set, NULL);
    pthread_create(&sig_thread, NULL, &signal_thread, (void *)&set);
#endif

    peer = ems_peer_create(role);

    EMSCommunicator *comm = ems_communicator_create(EMS_COMM_TYPE_UNIX,
                                                    "socket", "/tmp/test-ems-sock",
                                                    "role", role,
                                                    NULL, NULL);
    ems_peer_add_communicator(peer, comm);
    ems_peer_connect(peer);

    if (peer->role == EMS_PEER_ROLE_MASTER) {
        fprintf(stderr, "I am the master. (%d)\n", getpid());
        /* We have no work yet. So just sleep to get the connection working. */
        sleep(3);

        msg = ems_message_new(EMS_TEST_MESSAGE_QUIT,
                              EMS_MESSAGE_RECIPIENT_ALL,
                              EMS_MESSAGE_RECIPIENT_MASTER,
                              NULL, NULL);
        ems_peer_send_message(peer, msg);
        ems_message_free(msg);

        sleep(1);
    }
    else {
        fprintf(stderr, "I am a slave. (%d)\n", getpid());
        
        ems_peer_wait_for_message(peer);
        fprintf(stderr, "%d received message, quitting\n", getpid());
    }

    ems_peer_destroy(peer);

    ems_cleanup();

    return 0;
}
