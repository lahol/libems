#include "ems-message.h"
#include "ems-memory.h"
#include "ems-util.h"
#include "ems-error.h"
#include <memory.h>
#include <stdarg.h>

/* FIXME: Use a better data structure here. For the moment, a list should suffice,
 * but we could think about some form of tree, possibly arranged in such a way that
 * more frequent messages are higher up the tree to reduce the absolute searching time.
 */
EMSList *msg_classes = NULL;

/* A magic 4 byte string indicating a message of this library. */
char msg_magic[] = "EMSG";

static inline
EMSMessageClass *_ems_message_type_get_class(uint32_t type)
{
    EMSList *tmp;
    for (tmp = msg_classes; tmp; tmp = tmp->next) {
        if (((EMSMessageClass *)tmp->data)->msgtype == type)
            return (EMSMessageClass *)tmp->data;
    }
    return NULL;
}

/* Register a new message type. The type id shall be a user definded constant, since we want
 * to use this by multiple, heterogenous hosts. Therefore, we cannot return the type id from
 * an internal register.
 * Return an error if the type is already there. Internal messages all have the high bit set,
 * so avoiding those ids, there should be no type clash.
 * The user has to provide the functions for encoding/decoding the message as well as
 * setting values by key.
 */
int ems_message_register_type(uint32_t type, EMSMessageClass *msg_class)
{
    if (_ems_message_type_get_class(type))
        return EMS_ERROR_MESSAGE_TYPE_EXISTS;

    EMSMessageClass *new_class = ems_alloc(sizeof(EMSMessageClass));
    if (msg_class) {
        *new_class = *msg_class;
    }
    else {
        memset(new_class, 0, sizeof(EMSMessageClass));
        new_class->size = sizeof(EMSMessage);
    }
    new_class->msgtype = type;

    msg_classes = ems_list_prepend(msg_classes, new_class);

    return EMS_OK;
}

void ems_message_types_clear(void)
{
    ems_list_free_full(msg_classes, (EMSDestroyNotifyFunc)ems_free);
    msg_classes = NULL;
}

void ems_messages_set_magic(char *magic)
{
    strncpy(msg_magic, magic, 4);
}

/* Create a new message of the given type, followed by the recipient and sender ids.
 * After this a “NULL, NULL”-terminated sequence of key/value-pairs may follow.
 */
EMSMessage *ems_message_new(uint32_t type, uint32_t recipient_id, uint32_t sender_id, ...)
{
    EMSMessageClass *cls = _ems_message_type_get_class(type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *msg = ems_alloc(cls->size);
    msg->type = type;
    msg->recipient_id = recipient_id;
    msg->sender_id = sender_id;

    va_list args;
    va_start(args, sender_id);

    char *key;
    void *val;

    if (cls->msg_set_value) {
        while ((key = va_arg(args, char *)) != NULL) {
            val = va_arg(args, void *);

            cls->msg_set_value(msg, key, val);
        }
    }

    va_end(args);

    return msg;
}

/* Set a value given by some key. */
void ems_message_set_value(EMSMessage *msg, const char *key, const void *value)
{
    if (ems_unlikely(!msg))
        return;

    EMSMessageClass *cls;

    if (!strcmp(key, "sender-id")) {
        msg->sender_id = EMS_UTIL_POINTER_TO_INT(value);
    }
    else if (!strcmp(key, "recipient-id")) {
        msg->recipient_id = EMS_UTIL_POINTER_TO_INT(value);
    }
    else {
        cls = _ems_message_type_get_class(msg->type);
        if (ems_unlikely(!cls) || !cls->msg_set_value)
            return;
        cls->msg_set_value(msg, key, value);
    }
}

/* Encode a message. This calls the function from the class or writes only the generic part. */
size_t ems_message_encode(EMSMessage *msg, uint8_t **buffer)
{
    if (!msg)
        return 0;
    EMSMessageClass *cls = _ems_message_type_get_class(msg->type);
    if (ems_unlikely(!cls))
        return 0;

    size_t buflen = EMS_MESSAGE_HEADER_SIZE + cls->min_payload;
    *buffer = ems_alloc(buflen);

    strncpy((char *)(*buffer), msg_magic, 4);
    ems_message_write_u32(*buffer, 4, msg->type);
    ems_message_write_u32(*buffer, 8, msg->recipient_id);
    ems_message_write_u32(*buffer, 12, msg->sender_id);
    ems_message_write_u32(*buffer, 16, 0);

    if (cls->msg_encode)
        return cls->msg_encode(msg, buffer, buflen);

    return buflen;
}

/* Decode a message. */
void ems_message_decode_payload(EMSMessage *msg, uint8_t *payload, size_t payload_size)
{
    EMSMessageClass *cls = _ems_message_type_get_class(msg->type);

    if (cls->msg_decode)
        cls->msg_decode(msg, payload, payload_size);
}

EMSMessage *ems_message_decode_header(uint8_t *buffer, size_t buflen, size_t *payload_size)
{
    if (buflen < EMS_MESSAGE_HEADER_SIZE || !buffer)
        return NULL;

    if (strncmp((char *)buffer, msg_magic, 4))
        return NULL;

    uint32_t type = ems_message_read_u32(buffer, 4);
    EMSMessageClass *cls = _ems_message_type_get_class(type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *msg = ems_alloc(cls->size);
    msg->type = type;
    msg->recipient_id = ems_message_read_u32(buffer, 8);
    msg->sender_id = ems_message_read_u32(buffer, 12);

    if (payload_size)
        *payload_size = (size_t)ems_message_read_u32(buffer, 16);

    return msg;
}

/* Free a message. */
void ems_message_free(EMSMessage *msg)
{
    EMSMessageClass *cls;
    if (msg) {
        cls = _ems_message_type_get_class(msg->type);
        if (cls && cls->msg_free)
            cls->msg_free(msg);
        else
            ems_free(msg);
    }
}

/* Copy a message. */
int ems_message_copy(EMSMessage *dst, EMSMessage *src)
{
    if (!dst || !src || dst->type != src->type)
        return EMS_ERROR_MESSAGE_TYPE_MISMATCH;

    dst->recipient_id = src->recipient_id;
    dst->sender_id = src->sender_id;

    EMSMessageClass *cls = _ems_message_type_get_class(src->type);
    if (cls && cls->msg_copy)
        cls->msg_copy(dst, src);

    return EMS_OK;
}

/* Duplicate a message. */
EMSMessage *ems_message_dup(EMSMessage *msg)
{
    if (ems_unlikely(!msg))
        return NULL;

    EMSMessageClass *cls = _ems_message_type_get_class(msg->type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *new_msg = ems_alloc(cls->size);
    new_msg->type = msg->type;

    ems_message_copy(new_msg, msg);

    return new_msg;
}
