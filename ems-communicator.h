/* Base class for all communicators.
 *
 * A communicator provides an abstraction for the different means of
 * connections between master and slave.
 */
#pragma once

#include "ems-message.h"
#include "ems-msg-queue.h"

typedef enum {
    EMS_COMM_TYPE_UNIX = 1,             /* The communication over a UNIX domain socket. */
    EMS_COMM_TYPE_INET,                 /* The communication over the internet. */
} EMSCommunicatorType;                  /* The implemented communicator types. */

typedef enum {
    EMS_COMM_STATUS_UNKNOWN     = 0,
    EMS_COMM_STATUS_INITIALIZED = 1,
    EMS_COMM_STATUS_CONNECTED,
} EMSCommunicatorStatus;                /* The status of the communicator. */

typedef struct _EMSCommunicator EMSCommunicator;

/* Type definitions for the communicator class. */
typedef int (*EMSCommunicatorConnect)(EMSCommunicator *);
typedef int (*EMSCommunicatorDisconnect)(EMSCommunicator *);
typedef int (*EMSCommunicatorSendMessage)(EMSCommunicator *, EMSMessage *);
typedef void (*EMSCommunicatorDestroy)(EMSCommunicator *);
typedef void (*EMSCommunicatorHandleInternalMessage)(EMSCommunicator *, EMSMessage *);
typedef void (*EMSCommunicatorCloseConnection)(EMSCommunicator *, uint32_t);
typedef void (*EMSCommunicatorFlushOutgoingMessages)(EMSCommunicator *);

#include "ems-peer.h"

struct _EMSCommunicator {
    /* The type of the communicator. */
    EMSCommunicatorType type;

    /* Role of the communicator, i.e., is this used for the master or a slave. */
    EMSPeerRole role;

    /* Handle of the peer this communicator belongs to. */
    EMSPeer *peer;

    /* The unique id of the corresponding peer in the network. */
    uint32_t peer_id;

    /* The number of open connections of this communicator. */
    uint32_t open_connection_count;

    /* Communicator-specific function to destroy the object and release all resources. */
    EMSCommunicatorDestroy destroy;

    /* Communicator-specific function to connect to master or set up the master. */
    EMSCommunicatorConnect connect;

    /* Communicator-specific function to disconnect from master or all slaves. */
    EMSCommunicatorDisconnect disconnect;

    /* Send a message to all connected peers. */
    EMSCommunicatorSendMessage send_message;

    /* Handle an internal, connection-related message. */
    EMSCommunicatorHandleInternalMessage handle_int_message;

    /* Close the connection to a given peer. */
    EMSCommunicatorCloseConnection close_connection;

    /* Send all outstanding outgoing messages. */
    EMSCommunicatorFlushOutgoingMessages flush_outgoing;

    /* The status of the communicator. */
    EMSCommunicatorStatus status;

    /* Message queues for outgoing and incoming messages. */
    EMSMessageQueue msg_queue_outgoing;
    EMSMessageQueue msg_queue_incoming;
};

/* Create a new communicator of the given type. */
EMSCommunicator *ems_communicator_create(EMSCommunicatorType type, ...);

/* Create a new communicator from a string definition. */
EMSCommunicator *ems_communicator_create_from_string(const char *desc);

/* Set the role of the communicator. */
void ems_communicator_set_role(EMSCommunicator *comm, EMSPeerRole role);

/* Destroy the communicator. */
void ems_communicator_destroy(EMSCommunicator *comm);

/* Inform the communicator and the peer about a new connection. */
void ems_communicator_add_connection(EMSCommunicator *comm);

/* Inform the communicator and the peer about a closed or lost connection. */
void ems_communicator_remove_connection(EMSCommunicator *comm);

/* Set up the connection of the communicator. */
int ems_communicator_connect(EMSCommunicator *comm);

/* Close all open connections of this communicator. */
int ems_communicator_disconnect(EMSCommunicator *comm);

/* Send a message to all connected peers. */
int ems_communicator_send_message(EMSCommunicator *comm, EMSMessage *msg);

/* Handle an internal message. */
void ems_communicator_handle_internal_message(EMSCommunicator *comm, EMSMessage *msg);

/* Close the connection to the specified peer. */
void ems_communicator_close_connection(EMSCommunicator *comm, uint32_t peer_id);

/* Set the status of the communicator. */
void ems_communicator_set_status(EMSCommunicator *comm, EMSCommunicatorStatus status);

/* Set the id of the peer, retrieved from the master. */
void ems_communicator_set_peer_id(EMSCommunicator *comm, uint32_t peer_id);

/* Send all outstanding outgoing messages. */
void ems_communicator_flush_outgoing_messages(EMSCommunicator *comm);
