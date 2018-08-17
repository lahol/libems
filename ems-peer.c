#include "ems-peer.h"
#include "ems-memory.h"
#include <memory.h>
#include "ems-messages-internal.h"

void _ems_peer_handle_internal_message(EMSPeer *peer, EMSMessage *msg);
void *ems_peer_check_messages(EMSPeer *peer);

EMSPeer *ems_peer_create(EMSPeerRole role)
{
    EMSPeer *peer = ems_alloc(sizeof(EMSPeer));
    memset(peer, 0, sizeof(EMSPeer));

    peer->is_alive = 1;

    peer->role = role;
    ems_message_queue_init(&peer->msgqueue);

    pthread_mutex_init(&peer->peer_lock, NULL);
    pthread_mutex_init(&peer->msg_available_lock, NULL);
    pthread_cond_init(&peer->msg_available_cond, NULL);


    if (pthread_create(&peer->check_message_thread, NULL,
                   (PThreadCallback)ems_peer_check_messages, (void *)peer) == 0) {
        peer->thread_running = 1;
    }

    return peer;
}

void ems_peer_destroy(EMSPeer *peer)
{
    if (!peer)
        return;

    EMSList *tmp;
    while (peer->communicators) {
        tmp = peer->communicators->next;
        ems_communicator_destroy((EMSCommunicator *)peer->communicators->data);
        ems_free(peer->communicators);
        peer->communicators = tmp;
    }

    if (peer->thread_running) {
        /* signal message to wake up thread */
        ems_peer_signal_new_message(peer);
        pthread_join(peer->check_message_thread, NULL);
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

/* Quit all communicators. */
void ems_peer_terminate(EMSPeer *peer)
{
    EMSList *tmp;
    pthread_mutex_lock(&peer->peer_lock);
    while (peer->communicators) {
        tmp = peer->communicators->next;
        ems_communicator_destroy((EMSCommunicator *)peer->communicators->data);
        ems_free(peer->communicators);
        peer->communicators = tmp;
    }
    peer->is_alive = 0;
    pthread_mutex_unlock(&peer->peer_lock);

    /* Wake up message loops. */
    ems_peer_signal_new_message(peer);
}

void ems_peer_shutdown(EMSPeer *peer)
{
    if (ems_unlikely(!peer))
        return;

    EMSMessage *msg;
    if (peer->role == EMS_PEER_ROLE_MASTER) {
        /* send term msg */
        msg = ems_message_new(__EMS_MESSAGE_TERM,
                              EMS_MESSAGE_RECIPIENT_ALL,
                              peer->id,
                              NULL, NULL);
        ems_peer_send_message(peer, msg);
        ems_message_free(msg);

        /* wait for messages until there are no open connectinons anymore */
        while (ems_peer_get_connection_count(peer)) {
            /* The connection could be decreased after the last call and the signal
             * for the message might got lost. Therefore use a timeout. */
            ems_peer_wait_for_message_timeout(peer, 500);
        }
    }
    else {
        msg = ems_message_new(__EMS_MESSAGE_LEAVE,
                              EMS_MESSAGE_RECIPIENT_MASTER,
                              peer->id,
                              NULL, NULL);
        ems_peer_send_message(peer, msg);
        ems_message_free(msg);

        /* wait for leave ack? */
    }

    ems_peer_terminate(peer);
}

uint32_t ems_peer_get_connection_count(EMSPeer *peer)
{
    if (ems_unlikely(!peer))
        return 0;

    uint32_t count = 0;
    pthread_mutex_lock(&peer->peer_lock);
    count = peer->connection_count;
    pthread_mutex_unlock(&peer->peer_lock);

    return count;
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
    EMSList *tmp;
    pthread_mutex_lock(&peer->peer_lock);

    peer->id = id;
    for (tmp = peer->communicators; tmp; tmp = tmp->next) {
        ems_communicator_set_peer_id((EMSCommunicator *)tmp->data, id);
    }

    pthread_mutex_unlock(&peer->peer_lock);
}

uint32_t ems_peer_get_id(EMSPeer *peer)
{
    return peer ? peer->id : EMS_MESSAGE_RECIPIENT_ALL;
}

void ems_peer_push_message(EMSPeer *peer, EMSMessage *msg)
{
    ems_message_queue_push_tail(&peer->msgqueue, msg);
    ems_peer_signal_new_message(peer);
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

void ems_peer_wait_for_message_timeout(EMSPeer *peer, uint32_t timeout_ms)
{
    struct timespec timeout;

    clock_gettime(CLOCK_REALTIME, &timeout);

    timeout.tv_sec += timeout_ms / 1000;
    timeout.tv_nsec += (timeout_ms % 1000) * 1e6;
    if (timeout.tv_nsec >= 1e6) {
        ++timeout.tv_sec;
        timeout.tv_nsec -= 1e6;
    }

    pthread_mutex_lock(&peer->msg_available_lock);
    pthread_cond_timedwait(&peer->msg_available_cond, &peer->msg_available_lock, &timeout);
    pthread_mutex_unlock(&peer->msg_available_lock);
}

/* Get messages but filter out internal messages. */
EMSMessage *ems_peer_get_message(EMSPeer *peer)
{
    EMSMessage *msg = NULL;
    while ((msg = ems_message_queue_pop_head(&peer->msgqueue)) &&
           EMS_MESSAGE_IS_INTERNAL(msg)) {
        _ems_peer_handle_internal_message(peer, msg);
    }

    return msg;
}

void ems_peer_add_communicator(EMSPeer *peer, EMSCommunicator *comm)
{
    if (!peer || !comm)
        return;

    pthread_mutex_lock(&peer->peer_lock);
    peer->communicators = ems_list_prepend(peer->communicators, comm);
    comm->peer = peer;
    ems_communicator_set_peer_id(comm, peer->id);
    pthread_mutex_unlock(&peer->peer_lock);
}

void _ems_peer_handle_internal_message(EMSPeer *peer, EMSMessage *msg)
{
    if (ems_unlikely(!msg))
        return;

    switch (msg->type) {
        case __EMS_MESSAGE_CONNECTION_ADD:
            pthread_mutex_lock(&peer->peer_lock);
            ++peer->connection_count;
            pthread_mutex_unlock(&peer->peer_lock);
            break;
        case __EMS_MESSAGE_CONNECTION_DEL:
            pthread_mutex_lock(&peer->peer_lock);
            --peer->connection_count;
            pthread_mutex_unlock(&peer->peer_lock);
            break;
        case __EMS_MESSAGE_TERM:
            {
                EMSMessage *reply = ems_message_new(__EMS_MESSAGE_TERM_ACK,
                                                    msg->sender_id,
                                                    peer->id,
                                                    NULL, NULL);
                ems_peer_send_message(peer, reply);
                ems_message_free(reply);

                ems_peer_terminate(peer);
            }
            break;
        default:
            break;
    }

    ems_message_free(msg);
}

static int _ems_peer_filter_internal_message(EMSMessage *msg, void *nil)
{
    return EMS_MESSAGE_IS_INTERNAL(msg) ? 0 : 1;
}

void *ems_peer_check_messages(EMSPeer *peer)
{
    EMSMessage *msg;
    while (peer->is_alive) {
        ems_peer_wait_for_message(peer);
        while ((msg = ems_message_queue_pop_matching(&peer->msgqueue,
                                                     (EMSMessageFilterFunc)_ems_peer_filter_internal_message,
                                                     NULL)) != NULL)
            _ems_peer_handle_internal_message(peer, msg);
    }

    peer->thread_running = 0;
    return NULL;
}
