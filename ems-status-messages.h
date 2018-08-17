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

/* Some (arbitrary) offset for user-defined messages. We need some non-internal messages
 * to propagate changes ourselves. */
#define EMS_MESSAGE_USER                   0x00000010


