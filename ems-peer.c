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

    pthread_mutex_init(&peer->peer_lock, NULL);
    pthread_mutex_init(&peer->msg_available_lock, NULL);
    pthread_cond_init(&peer->msg_available_cond, NULL);

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

    pthread_mutex_destroy(&peer->peer_lock);
    pthread_mutex_destroy(&peer->msg_available_lock);
    pthread_cond_destroy(&peer->msg_available_cond);

    ems_free(peer);
}

void ems_peer_connect(EMSPeer *peer)
{
    EMSList *tmp;
    pthread_mutex_lock(&peer->peer_lock);
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_connect((EMSCommunicator *)tmp->data);
    }
    pthread_mutex_unlock(&peer->peer_lock);
}

void ems_peer_disconnect(EMSPeer *peer)
{
    EMSList *tmp;
    pthread_mutex_lock(&peer->peer_lock);
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_disconnect((EMSCommunicator *)tmp->data);
    }
    pthread_mutex_unlock(&peer->peer_lock);
}

void ems_peer_send_message(EMSPeer *peer, EMSMessage *msg)
{
    EMSList *tmp;
    pthread_mutex_lock(&peer->peer_lock);
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_send_message((EMSCommunicator *)tmp->data, msg);
    }
    pthread_mutex_unlock(&peer->peer_lock);
}

void ems_peer_shutdown(EMSPeer *peer)
{
    if (peer && peer->shutdown)
        peer->shutdown(peer);
}

uint32_t ems_peer_generate_new_slave_id(EMSPeer *peer)
{
    /* FIXME: lock/unlock, may be called by different threads for multiple communicators */
    uint32_t new_id;
    pthread_mutex_lock(&peer->peer_lock);
    new_id = ++peer->max_slave_id;
    pthread_mutex_unlock(&peer->peer_lock);

    return new_id;
}

void ems_peer_set_id(EMSPeer *peer, uint32_t id)
{
    fprintf(stderr, "got own id %d (%d)\n", id, getpid());
    peer->id = id;
}

void ems_peer_push_message(EMSPeer *peer, EMSMessage *msg)
{
    if (EMS_MESSAGE_IS_INTERNAL(msg)) {
        /* handle internal message */
    }
    else {
        ems_message_queue_push_tail(&peer->msgqueue, msg);

        ems_peer_signal_new_message(peer);
    }
}

void ems_peer_signal_new_message(EMSPeer *peer)
{
    pthread_mutex_lock(&peer->msg_available_lock);
    pthread_cond_broadcast(&peer->msg_available_cond);
    pthread_mutex_unlock(&peer->msg_available_lock);
}

void ems_peer_wait_for_message(EMSPeer *peer)
{
    pthread_mutex_lock(&peer->msg_available_lock);
    pthread_cond_wait(&peer->msg_available_cond, &peer->msg_available_lock);
    pthread_mutex_unlock(&peer->msg_available_lock);
}

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm)
{
    if (!peer || !comm)
        return;

    pthread_mutex_lock(&peer->peer_lock);
    peer->communicators = ems_list_prepend(peer->communicators, comm);
    comm->peer = peer;
    pthread_mutex_unlock(&peer->peer_lock);
}
