/* Some messages used for status updates. */
#pragma once

#include "ems-message.h"
#include "ems-peer.h"

/* Something in the peer has changed (connections, …)
 * This is only used locally and not sent over the net.
 */
#define EMS_MESSAGE_STATUS_PEER_CHANGED    0x00000001

#define EMS_PEER_STATUS_CONNECTION_ADD     1
#define EMS_PEER_STATUS_CONNECTION_DEL     2
#define EMS_PEER_STATUS_ID_CHANGED         3
typedef struct {
    EMSMessage parent;
    EMSPeer *peer;
    uint32_t peer_status;
    uint32_t remote_id;         /* set if connection is deleted */
} EMSMessageStatusPeerChanged;

/* The peer is connected and ready to work.
 */
#define EMS_MESSAGE_STATUS_PEER_READY      0x00000002
typedef struct {
    EMSMessage parent;
    EMSPeer *peer;
} EMSMessageStatusPeerReady;
