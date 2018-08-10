#pragma once

#include "ems-msg-queue.h"
#include "ems-communicator.h"
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
};

EMSPeer *ems_peer_create(EMSPeerRole role);
void ems_peer_destroy(EMSPeer *peer);

void ems_peer_send_message(EMSPeer *peer, EMSMessage *msg);
void ems_peer_shutdown(EMSPeer *peer);

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm);
