#pragma once

#include "ems-msg-queue.h"
#include "ems-util.h"
#include <stdint.h>

typedef enum {
    EMS_PEER_ROLE_MASTER = 0,
    EMS_PEER_ROLE_SLAVE  = 1
} EMSPeerRole;

typedef struct _EMSPeer EMSPeer;

typedef void (*PeerSendMessage)(EMSPeer *, EMSMessage *msg);
typedef void (*PeerShutdown)(EMSPeer *);
typedef void (*PeerDestroy)(EMSPeer *);

struct _EMSPeer {
    EMSPeerRole role;
    uint32_t id;
    EMSMessageQueue msgqueue;

    EMSList *communicators; /* EMSCommunicator */

    /* msghandler() */
    PeerSendMessage send_message;
    PeerShutdown shutdown;
    PeerDestroy destroy;

    uint32_t max_slave_id;
    uint32_t connection_count;

    pthread_mutex_t peer_lock;
    pthread_mutex_t msg_available_lock;
    pthread_cond_t  msg_available_cond;

    unsigned int is_alive : 1;
};

EMSPeer *ems_peer_create(EMSPeerRole role);
void ems_peer_destroy(EMSPeer *peer);

void ems_peer_connect(EMSPeer *peer);
void ems_peer_disconnect(EMSPeer *peer);

void ems_peer_send_message(EMSPeer *peer, EMSMessage *msg);
void ems_peer_shutdown(EMSPeer *peer);

uint32_t ems_peer_get_connection_count(EMSPeer *peer);

uint32_t ems_peer_generate_new_slave_id(EMSPeer *peer);
void ems_peer_set_id(EMSPeer *peer, uint32_t id);
uint32_t ems_peer_get_id(EMSPeer *peer);
void ems_peer_push_message(EMSPeer *peer, EMSMessage *msg);
void ems_peer_signal_new_message(EMSPeer *peer);
void ems_peer_wait_for_message(EMSPeer *peer);
void ems_peer_wait_for_message_timeout(EMSPeer *peer, uint32_t timeout_ms);

#include "ems-communicator.h"

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm);
