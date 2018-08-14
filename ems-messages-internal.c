#include "ems-messages-internal.h"
#include "ems-message.h"
#include "ems-util.h"
#include "ems-memory.h"
#include "ems-error.h"
#include <string.h>

/* __EMS_MESSAGE_TYPE_SET_ID */
size_t _ems_message_int_set_id_encode(EMSMessage *msg, uint8_t **buffer, size_t buflen)
{
    if (ems_unlikely(buflen < EMS_MESSAGE_HEADER_SIZE + 4))
        *buffer = ems_realloc(*buffer, EMS_MESSAGE_HEADER_SIZE + 4);
    /* payload_size */
    ems_message_write_u32(*buffer, 16, 4);

    /* the actual data */
    ems_message_write_u32(*buffer, 20, ((EMSMessageIntSetId *)msg)->peer_id);

    return EMS_MESSAGE_HEADER_SIZE + 4;
}

void _ems_message_int_set_id_decode(EMSMessage *msg, uint8_t *payload, size_t buflen)
{
    if (ems_unlikely(buflen < 4)) {
        ((EMSMessageIntSetId *)msg)->peer_id = 0;
        return;
    }

    ((EMSMessageIntSetId *)msg)->peer_id = ems_message_read_u32(payload, 0);
}

void _ems_message_int_set_id_set_value(EMSMessage *msg, const char *key, const void *value)
{
    if (!strcmp(key, "peer-id")) {
        ((EMSMessageIntSetId *)msg)->peer_id = EMS_UTIL_POINTER_TO_INT(value);
    }
}

void _ems_message_int_set_id_copy(EMSMessage *dst, EMSMessage *src)
{
    ((EMSMessageIntSetId *)dst)->peer_id = ((EMSMessageIntSetId *)src)->peer_id;
}

/* __EMS_MESSAGE_TYPE_LEAVE */
/* nothing special needed */

int ems_messages_register_internal_types(void)
{
    EMSMessageClass msgclass;
    int rc;

    /* __EMS_MESSAGE_TYPE_SET_ID */
    msgclass.msgtype = __EMS_MESSAGE_TYPE_SET_ID;
    msgclass.size = sizeof(EMSMessageIntSetId);
    msgclass.min_payload = 4;
    msgclass.msg_encode = _ems_message_int_set_id_encode;
    msgclass.msg_decode = _ems_message_int_set_id_decode;
    msgclass.msg_set_value = _ems_message_int_set_id_set_value;
    msgclass.msg_copy = _ems_message_int_set_id_copy;
    msgclass.msg_free = NULL;

    if ((rc = ems_message_register_type(__EMS_MESSAGE_TYPE_SET_ID, &msgclass)) != EMS_OK)
        return rc;

    /* __EMS_MESSAGE_TYPE_LEAVE */
    msgclass.msgtype = __EMS_MESSAGE_TYPE_LEAVE;
    msgclass.size = sizeof(EMSMessageIntLeave);
    msgclass.min_payload = 0;
    msgclass.msg_encode = NULL;
    msgclass.msg_decode = NULL;
    msgclass.msg_set_value = NULL;
    msgclass.msg_copy = NULL;
    msgclass.msg_free = NULL;

    if ((rc = ems_message_register_type(__EMS_MESSAGE_TYPE_LEAVE, &msgclass)) != EMS_OK)
        return rc;

    return EMS_OK;
}
