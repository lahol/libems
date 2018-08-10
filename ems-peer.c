#include "ems-peer.h"
#include "ems-memory.h"
#include <memory.h>

EMSPeer *ems_peer_create(EMSPeerRole role)
{
    EMSPeer *peer = ems_alloc(sizeof(EMSPeer));
    memset(peer, 0, sizeof(EMSPeer));

    peer->role = role;
    ems_message_queue_init(&peer->msgqueue);

    return peer;
}

void ems_peer_destroy(EMSPeer *peer)
{
    if (!peer)
        return;

    if (peer->destroy)
        peer->destroy(peer);

    EMSList *tmp;
    while (peer->communicators) {
        tmp = peer->communicators->next;
        ems_communicator_destroy((EMSCommunicator *)peer->communicators->data);
        ems_free(peer->communicators);
        peer->communicators = tmp;
    }

    ems_message_queue_clear(&peer->msgqueue);

    ems_free(peer);
}

void ems_peer_send_message(EMSPeer *peer, EMSMessage *msg)
{
    if (peer && peer->send_message)
        peer->send_message(peer, msg);
}

void ems_peer_shutdown(EMSPeer *peer)
{
    if (peer && peer->shutdown)
        peer->shutdown(peer);
}

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm)
{
    if (!peer || !comm)
        return;

    peer->communicators = ems_list_prepend(peer->communicators, comm);
}
