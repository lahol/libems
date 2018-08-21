/* Some messages used for status updates. */
#pragma once

#include "ems-message.h"
#include "ems-peer.h"

/* Something in the peer has changed (connections, …)
 * This is only used locally and not sent over the net.
 */
#define EMS_MESSAGE_STATUS_PEER_CHANGED    0x00000001
typedef struct {
    EMSMessage parent;
    EMSPeer *peer;
} EMSMessageStatusPeerChanged;

/* The peer is connected and ready to work.
 */
#define EMS_MESSAGE_STATUS_PEER_READY      0x00000002
typedef struct {
    EMSMessage parent;
    EMSPeer *peer;
} EMSMessageStatusPeerReady;
