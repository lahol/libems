#include "ems-messages-internal.h"
#include "ems-message.h"
#include "ems-status-messages.h"
#include "ems-util.h"
#include "ems-memory.h"
#include "ems-error.h"
#include <string.h>
#include <stddef.h>

/* __EMS_MESSAGE_SET_ID */
size_t _ems_message_int_set_id_encode(EMSMessage *msg, uint8_t **buffer, size_t buflen)
{
    if (ems_unlikely(buflen < EMS_MESSAGE_HEADER_SIZE + 8))
        *buffer = ems_realloc(*buffer, EMS_MESSAGE_HEADER_SIZE + 8);
    /* payload_size */
    ems_message_write_payload_size(*buffer, 8);

    /* the actual data */
    ems_message_write_u64(*buffer, EMS_MESSAGE_HEADER_SIZE, ((EMSMessageIntSetId *)msg)->peer_id);

    return EMS_MESSAGE_HEADER_SIZE + 8;
}

void _ems_message_int_set_id_decode(EMSMessage *msg, uint8_t *payload, size_t buflen)
{
    if (ems_unlikely(buflen < 8)) {
        ((EMSMessageIntSetId *)msg)->peer_id = 0;
        return;
    }

    ((EMSMessageIntSetId *)msg)->peer_id = ems_message_read_u64(payload, 0);
}

void _ems_message_int_set_id_copy(EMSMessage *dst, EMSMessage *src)
{
    ((EMSMessageIntSetId *)dst)->peer_id = ((EMSMessageIntSetId *)src)->peer_id;
}

/* __EMS_MESSAGE_LEAVE */
/* nothing to do here */

/* __EMS_MESSAGE_TERM */
/* nothing to do here */

/* __EMS_MESSAGE_TERM_ACK */
/* nothing to do here */

/* __EMS_MESSAGE_CONNECTION_ADD */
/* nothing to do here */

/* __EMS_MESSAGE_CONNECTION_DEL */
size_t _ems_message_int_connection_del_encode(EMSMessage *msg, uint8_t **buffer, size_t buflen)
{
    if (ems_unlikely(buflen < EMS_MESSAGE_HEADER_SIZE + 8))
        *buffer = ems_realloc(*buffer, EMS_MESSAGE_HEADER_SIZE + 8);

    ems_message_write_payload_size(*buffer, 8);

    ems_message_write_u64(*buffer, EMS_MESSAGE_HEADER_SIZE, ((EMSMessageIntConnectionDel *)msg)->remote_id);

    return EMS_MESSAGE_HEADER_SIZE + 8;
}

void _ems_message_int_connection_del_decode(EMSMessage *msg, uint8_t *payload, size_t buflen)
{
    if (ems_unlikely(buflen < 8)) {
        ((EMSMessageIntConnectionDel *)msg)->remote_id = 0;
        return;
    }

    ((EMSMessageIntConnectionDel *)msg)->remote_id = ems_message_read_u64(payload, 0);
}

void _ems_message_int_connection_del_copy(EMSMessage *dst, EMSMessage *src)
{
    ((EMSMessageIntConnectionDel *)dst)->remote_id = ((EMSMessageIntConnectionDel *)src)->remote_id;
}

/* EMS_MESSAGE_STATUS_PEER_CHANGED */
void _ems_messages_status_peer_changed_set_value(EMSMessage *msg, const char *key, const void *value)
{
    if (!strcmp(key, "peer")) {
        ((EMSMessageStatusPeerChanged *)msg)->peer = (EMSPeer *)value;
    }
}

void _ems_messages_status_peer_changed_copy(EMSMessage *dst, EMSMessage *src)
{
    ((EMSMessageStatusPeerChanged *)dst)->peer = ((EMSMessageStatusPeerChanged *)src)->peer;
    ((EMSMessageStatusPeerChanged *)dst)->peer_status = ((EMSMessageStatusPeerChanged *)src)->peer_status;
    ((EMSMessageStatusPeerChanged *)dst)->remote_id = ((EMSMessageStatusPeerChanged *)src)->remote_id;
}

/* EMS_MESSAGE_STATUS_PEER_READY */
void _ems_messages_status_peer_ready_copy(EMSMessage *dst, EMSMessage *src)
{
    ((EMSMessageStatusPeerReady *)dst)->peer = ((EMSMessageStatusPeerReady *)src)->peer;
}

/* Register the internal messages. */
int ems_messages_register_internal_types(void)
{
    EMSMessageClass msgclass;
    int rc;

    /* __EMS_MESSAGE_SET_ID */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_SET_ID;
    msgclass.size          = sizeof(EMSMessageIntSetId);
    msgclass.min_payload   = 8;
    msgclass.msg_encode    = _ems_message_int_set_id_encode;
    msgclass.msg_decode    = _ems_message_int_set_id_decode;
    msgclass.msg_copy      = _ems_message_int_set_id_copy;

    if ((rc = ems_message_register_type(__EMS_MESSAGE_SET_ID, &msgclass)) != EMS_OK)
        return rc;

    ems_message_type_add_member(__EMS_MESSAGE_SET_ID,
                                EMS_MSG_MEMBER_UINT64,
                                0,
                                "peer-id",
                                offsetof(EMSMessageIntSetId, peer_id),
                                NULL);

    /* __EMS_MESSAGE_LEAVE */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_LEAVE;
    msgclass.size          = sizeof(EMSMessageIntLeave);

    if ((rc = ems_message_register_type(__EMS_MESSAGE_LEAVE, &msgclass)) != EMS_OK)
        return rc;

    /* __EMS_MESSAGE_TERM */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_TERM;
    msgclass.size          = sizeof(EMSMessageIntTerm);

    if ((rc = ems_message_register_type(__EMS_MESSAGE_TERM, &msgclass)) != EMS_OK)
        return rc;

    /* __EMS_MESSAGE_TERM_ACK */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_TERM_ACK;
    msgclass.size          = sizeof(EMSMessageIntTermAck);

    if ((rc = ems_message_register_type(__EMS_MESSAGE_TERM_ACK, &msgclass)) != EMS_OK)
        return rc;

    /* __EMS_MESSAGE_CONNECTION_ADD */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_CONNECTION_ADD;
    msgclass.size          = sizeof(EMSMessageIntConnectionAdd);

    if ((rc = ems_message_register_type(__EMS_MESSAGE_CONNECTION_ADD, &msgclass)) != EMS_OK)
        return rc;

    /* __EMS_MESSAGE_CONNECTION_DEL */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_CONNECTION_DEL;
    msgclass.size          = sizeof(EMSMessageIntConnectionDel);
    msgclass.min_payload   = 8;
    msgclass.msg_encode    = _ems_message_int_connection_del_encode;
    msgclass.msg_decode    = _ems_message_int_connection_del_decode;
    msgclass.msg_copy      = _ems_message_int_connection_del_copy;

    if ((rc = ems_message_register_type(__EMS_MESSAGE_CONNECTION_DEL, &msgclass)) != EMS_OK)
        return rc;

    ems_message_type_add_member(__EMS_MESSAGE_CONNECTION_DEL,
                                EMS_MSG_MEMBER_UINT64,
                                0,
                                "remote-id",
                                offsetof(EMSMessageIntConnectionDel, remote_id),
                                NULL);

    /* __EMS_MESSAGE_QUEUE_DISABLED */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = __EMS_MESSAGE_QUEUE_DISABLED;
    msgclass.size          = sizeof(EMSMessageQueueDisabled);

    if ((rc = ems_message_register_type(__EMS_MESSAGE_QUEUE_DISABLED, &msgclass)) != EMS_OK)
        return rc;

    /* EMS_MESSAGE_STATUS_PEER_CHANGED */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = EMS_MESSAGE_STATUS_PEER_CHANGED;
    msgclass.size          = sizeof(EMSMessageStatusPeerChanged);
    msgclass.msg_copy      = _ems_messages_status_peer_changed_copy;

    if ((rc = ems_message_register_type(EMS_MESSAGE_STATUS_PEER_CHANGED, &msgclass)) != EMS_OK)
        return rc;

    ems_message_type_add_member(EMS_MESSAGE_STATUS_PEER_CHANGED,
                                EMS_MSG_MEMBER_UINT,
                                0,
                                "peer",
                                offsetof(EMSMessageStatusPeerChanged, peer),
                                NULL);
    ems_message_type_add_member(EMS_MESSAGE_STATUS_PEER_CHANGED,
                                EMS_MSG_MEMBER_UINT,
                                1,
                                "peer-status",
                                offsetof(EMSMessageStatusPeerChanged, peer_status),
                                NULL);
    ems_message_type_add_member(EMS_MESSAGE_STATUS_PEER_CHANGED,
                                EMS_MSG_MEMBER_UINT,
                                2,
                                "remote-id",
                                offsetof(EMSMessageStatusPeerChanged, remote_id),
                                NULL);

    /* EMS_MESSAGE_STATUS_PEER_READY */
    memset(&msgclass, 0, sizeof(EMSMessageClass));
    msgclass.msgtype       = EMS_MESSAGE_STATUS_PEER_READY;
    msgclass.size          = sizeof(EMSMessageStatusPeerReady);
    msgclass.msg_copy      = _ems_messages_status_peer_ready_copy;

    if ((rc = ems_message_register_type(EMS_MESSAGE_STATUS_PEER_READY, &msgclass)) != EMS_OK)
        return rc;

    ems_message_type_add_member(EMS_MESSAGE_STATUS_PEER_READY,
                                EMS_MSG_MEMBER_UINT,
                                0,
                                "peer",
                                offsetof(EMSMessageStatusPeerReady, peer),
                                NULL);


    return EMS_OK;
}
