#include "ems-peer.h"
#include "ems-memory.h"
#include <memory.h>
#include <stdio.h>

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

void ems_peer_connect(EMSPeer *peer)
{
    EMSList *tmp;
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_connect((EMSCommunicator *)tmp->data);
    }
}

void ems_peer_disconnect(EMSPeer *peer)
{
    EMSList *tmp;
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_disconnect((EMSCommunicator *)tmp->data);
    }
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

uint32_t ems_peer_query_slave_id(EMSCommunicator *comm, EMSPeer *peer)
{
    /* FIXME: lock/unlock, may be called by different threads for multiple communicators */
    return ++peer->max_slave_id;
}

void ems_peer_set_own_id(EMSCommunicator *comm, uint32_t id, EMSPeer *peer)
{
    fprintf(stderr, "got own id %d (%d)\n", id, getpid());
    peer->id = id;
}

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm)
{
    if (!peer || !comm)
        return;

    EMSCommunicatorCallbacks callbacks = {
        .query_slave_id = (EMSCommunicatorQuerySlaveId)ems_peer_query_slave_id,
        .set_own_id = (EMSCommunicatorSetOwnId)ems_peer_set_own_id,
        .user_data = peer
    };

    ems_communicator_set_callbacks(comm, &callbacks);
    peer->communicators = ems_list_prepend(peer->communicators, comm);
}
