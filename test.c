#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ems.h"
#include <getopt.h>
#include <string.h>

#define EMS_TEST_MESSAGE_QUIT      (EMS_MESSAGE_USER + 1)

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

EMSPeer *peer = NULL;
EMSPeerRole role = EMS_PEER_ROLE_MASTER;

char *cfg_hostname = NULL;
uint16_t cfg_port = 0;
char *cfg_unix_socket = NULL;
int cfg_slave_only = 0;
int cfg_slave_count = 1;


#if 1
#include <pthread.h>
#include <signal.h>

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

        if (sig == SIGINT) {
            /* lock? */
            fprintf(stderr, "%d Caught SIGINT\n", getpid());
            if (cfg_slave_only || role == EMS_PEER_ROLE_MASTER) {
                ems_peer_shutdown(peer);
                fprintf(stderr, "%d peer disconnect done\n", getpid());
            }
        }
    }

    return NULL;
}
#endif

int parse_options(int argc, char **argv)
{
    static struct option long_options[] = {
        { "hostname", required_argument, 0, 'h' },
        { "port", required_argument, 0, 'p' },
        { "fifo", required_argument, 0, 'u' },
        { "slave", no_argument, &cfg_slave_only, 1 },
        { "count", required_argument, 0, 'c' },
        { 0, 0, 0, 0 },
    };

    int c;
    int option_index = 0;

    /* This is just to give an idea how to use the library. No correct error checking,
     * multiple connections, or something like this is done. */
    while (1) {
        c = getopt_long(argc, argv, "h:p:u:c:", long_options, &option_index);

        if (c == -1)
            break;
        switch (c) {
            case 0:
/*                if (long_options[option_index].flag != 0)
                    break;*/
                break;
            case 'h':
                cfg_hostname = strdup(optarg);
                break;
            case 'p':
                cfg_port = (uint16_t)strtoul(optarg, NULL, 10);
                break;
            case 'u':
                cfg_unix_socket = strdup(optarg);
                break;
            case 'c':
                cfg_slave_count = strtoul(optarg, NULL, 10);
                break;
            case '?':
                break;
            default:
                return 1;
        }
    }
    return 0;
}

void handle_peer_message(EMSPeer *peer, EMSMessage *msg, void *userdata)
{
    fprintf(stderr, "%d %s received msg 0x%08x, peer_id: %d\n",
            getpid(),
            peer->role == EMS_PEER_ROLE_MASTER ? "MASTER" : "SLAVE",
            msg->type,
            ems_peer_get_id(peer));
}

int main(int argc, char **argv)
{
    int j;

    pid_t pid;

    if (parse_options(argc, argv) != 0) {
        fprintf(stderr, "Error parsing options.\n");
        return 1;
    }

    fprintf(stderr, "host: %s:%u\nsocket: %s\ncount: %d\nslave-only: %d\n",
            cfg_hostname, cfg_port, cfg_unix_socket, cfg_slave_count, cfg_slave_only);

    ems_init("EMSG");

    test_register_messages();

    role = cfg_slave_only ? EMS_PEER_ROLE_SLAVE : EMS_PEER_ROLE_MASTER;
    if (cfg_slave_only && !cfg_slave_count)
        cfg_slave_count = 1;


    for (j = cfg_slave_only ? 1 : 0; j < cfg_slave_count; ++j) {
        pid = fork();
        if (pid != 0) {
            fprintf(stderr, "forked: %d\n", pid);
        }
        else {
            role = EMS_PEER_ROLE_SLAVE;
            break;
        }
    }

#if 1
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

    EMSCommunicator *comm = NULL;


    if (cfg_unix_socket) {
        comm = ems_communicator_create(EMS_COMM_TYPE_UNIX,
                                       "socket", cfg_unix_socket,
                                       "role", role,
                                       NULL, NULL);
        ems_peer_add_communicator(peer, comm);
    }
                                                    
    if (cfg_hostname || cfg_port) {
        /* only if no unix_socket || role master otherwise the slaves connect twice */
        if (!cfg_unix_socket || role == EMS_PEER_ROLE_MASTER) {
            comm = ems_communicator_create(EMS_COMM_TYPE_INET,
                                           "hostname", cfg_hostname,
                                           "port", cfg_port,
                                           "role", role,
                                           NULL, NULL);
            ems_peer_add_communicator(peer, comm);
        }
    }

    ems_peer_connect(peer);

    if (peer->role == EMS_PEER_ROLE_MASTER) {
        fprintf(stderr, "I am the master. (%d)\n", getpid());
    }
    else {
        fprintf(stderr, "I am a slave. (%d)\n", getpid());
    }

    ems_peer_start_event_loop(peer, handle_peer_message, NULL, 0);

    ems_peer_destroy(peer);

    ems_cleanup();

    return 0;
}
