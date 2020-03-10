/* Main entrance for communication.
 *
 * Each peer may have multiple communicators, used for different or even the same means
 * of types of connections. For example, the master can listen to UNIX domain sockets
 * and internet connections, possibly on multiple ports.
 * However, we advise you to only use multiple communicators for the master, since it
 * makes no sense that a slave has multiple connections.
 * For the future, it is planned that the library does not allow this on its own.
 * But for the moment, you have to take care of this yourself. See the test for
 * an example.
 *
 * The peer has a list of communicators, which are responsible for the connection to
 * the other peers/slaves or to the master. Each peer has an unique id, the master
 * being 0. Messages can be sent to all peers or just to one specific peer.
 *
 * Messages are retrieved by ems_peer_get_message. You can wait for incoming messages
 * with ems_peer_wait_for_message or ems_peer_wait_for_message_timeout as long
 * as the peer is alive.
 */
#pragma once

#include "ems-msg-queue.h"
#include "ems-util.h"
#include <stdint.h>

/* The role of this peer. It can either be the master or a slave. */
typedef enum {
    EMS_PEER_ROLE_MASTER = 0,
    EMS_PEER_ROLE_SLAVE  = 1
} EMSPeerRole;

typedef struct _EMSPeer EMSPeer;

struct _EMSPeer {
    /* The role of this peer. Either master or slave. */
    EMSPeerRole role;

    /* The id identifying this peer in the network. */
    uint64_t id;

    /* The message queue for incoming messages. */
    EMSMessageQueue msgqueue;

    /* The list of all installed communicators. */
    EMSList *communicators; /* EMSCommunicator */

    /* The last id given to a peer. Each new peer gets a message from the
     * master informing it about its id. */
    uint64_t max_slave_id;

    /* The number of open connections. This may change with new incoming connections
     * or if slaves leave, or there is an error in the connection. */
    uint32_t connection_count;

    /* Locks for the data and the condition of available messages. */
    pthread_mutex_t peer_lock;
    pthread_mutex_t msg_available_lock;
    pthread_cond_t  msg_available_cond;

    /* Flag indicating that the peer is alive. In a dead peer no more messages are
     * received.
     */
    unsigned int is_alive : 1;

    /* Flag indicating that the message queue thread is running. */
    unsigned int msg_thread_running : 1;

    /* Flag indicating that checking for messages is enabled. */
    unsigned int msg_thread_enabled : 1;

    /* The thread checking for internal messages. */
    pthread_t check_message_thread;
    pthread_t event_loop;
};

/* Create a new peer of the specified role. */
EMSPeer *ems_peer_create(EMSPeerRole role);

/* Destroy the peer and free all resources. */
void ems_peer_destroy(EMSPeer *peer);

/* Inform the communicators to connect to the master or wait for incoming connections. */
void ems_peer_connect(EMSPeer *peer);

/* Disconnect all communicators. */
void ems_peer_disconnect(EMSPeer *peer);

/* Send a message to one or all connected peers. */
void ems_peer_send_message(EMSPeer *peer, EMSMessage *msg);

/* Shutdown the peer. This stops all communicators, informing all connected peers about
 * this in advance and waits until all connections are closed.
 * The peer is marked as dead afterwards and cannot be made alive again.
 */
void ems_peer_shutdown(EMSPeer *peer);

/* Flush outgoing messages on all communicators. */
void ems_peer_flush_outgoing_messages(EMSPeer *peer);

/* Get the number of open connections of this peer. */
uint32_t ems_peer_get_connection_count(EMSPeer *peer);

/* Request a new identifier for a slave. */
uint64_t ems_peer_generate_new_slave_id(EMSPeer *peer);

/* Set the peer’s own id. */
void ems_peer_set_id(EMSPeer *peer, uint64_t id);

/* Retrieve the id of this peer. */
uint64_t ems_peer_get_id(EMSPeer *peer);

/* Push an internal message to the message queue and inform all waiting threads
 * about this.
 */
void ems_peer_push_message(EMSPeer *peer, EMSMessage *msg);

/* Signal the arrival of a new message in the queue. */
void ems_peer_signal_new_message(EMSPeer *peer);

/* Wait for new messages.
 * This blocks the execution of the calling thread until a new message arrives.
 */
void ems_peer_wait_for_message(EMSPeer *peer);

/* Wait for new messages or a timeout.
 * This blocks the execution of the calling thread until a new message arrives or
 * the timeout (in milliseconds) is expired.
 */
void ems_peer_wait_for_message_timeout(EMSPeer *peer, uint32_t timeout_ms);

/* Retrieve a message from the peer.
 * Internally, this checks first if the next message is for internal use only
 * and handles it if this is the case.
 */
/* FIXME: get message with (timed) wait? -> no, want still just peek for messages */
EMSMessage *ems_peer_get_message(EMSPeer *peer);

#include "ems-communicator.h"

/* Add a communicator to the peer. */
void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm);

/* Run a event loop waiting for messages in its own thread and call event_cb on new messages. */
typedef void (*EMSPeerEventCallback)(EMSPeer *, EMSMessage *, void *);
void ems_peer_start_event_loop(EMSPeer *peer, EMSPeerEventCallback event_cb, void *userdata, int do_return);

/* Stop a possible event loop */
void ems_peer_stop_event_loop(EMSPeer *peer);

