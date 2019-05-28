#include "ems-message.h"
#include "ems-memory.h"
#include "ems-util.h"
#include "ems-error.h"
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    EMSMessageMemberType type;
    uint32_t id;
    char     *name;
    size_t   offset;
    EMSMessageClassSetMemberCallback set_cb;
} EMSMessageClassMember;

typedef struct {
    EMSMessageClass klass;
    EMSList *members;        /* [EMSMessageClassMember] */
} EMSMessageClassInternal;

/* FIXME: Use a better data structure here. For the moment, a list should suffice,
 * but we could think about some form of tree, possibly arranged in such a way that
 * more frequent messages are higher up the tree to reduce the absolute searching time.
 */
EMSList *msg_classes = NULL;

/* A magic 4 byte string indicating a message of this library. */
char msg_magic[] = "EMSG";

static inline
EMSMessageClassInternal *_ems_message_type_get_class(uint32_t type)
{
    EMSList *tmp;
    for (tmp = msg_classes; tmp; tmp = tmp->next) {
        if (((EMSMessageClass *)tmp->data)->msgtype == type)
            return (EMSMessageClassInternal *)tmp->data;
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

    EMSMessageClassInternal *new_class = ems_alloc(sizeof(EMSMessageClassInternal));
    if (msg_class) {
        *((EMSMessageClass *)new_class) = *msg_class;
        new_class->members = NULL;
    }
    else {
        memset(new_class, 0, sizeof(EMSMessageClass));
        new_class->klass.size = sizeof(EMSMessage);
    }
    new_class->klass.msgtype = type;

    msg_classes = ems_list_prepend(msg_classes, new_class);

    return EMS_OK;
}

int ems_message_type_add_member(uint32_t msgtype,
                                EMSMessageMemberType member_type,
                                uint32_t member_id,
                                const char *member_name,
                                size_t member_offset,
                                EMSMessageClassSetMemberCallback member_set_cb)
{
    EMSList *tmp;
    EMSMessageClassMember *member;
    if (!member_name)
        return EMS_ERROR_INVALID_ARGUMENT;

    EMSMessageClassInternal *cls = _ems_message_type_get_class(msgtype);
    if (!cls)
        return EMS_ERROR_INVALID_ARGUMENT;

    for (tmp = cls->members; tmp; tmp = tmp->next) {
        if (!strcmp(((EMSMessageClassMember *)tmp->data)->name, member_name))
            return EMS_ERROR_MEMBER_ALREADY_PRESENT;
    }

    member = ems_alloc(sizeof(EMSMessageClassMember));
    member->type   = member_type;
    member->id     = member_id;
    member->offset = member_offset;
    member->set_cb = member_set_cb;
    member->name   = strdup(member_name);

    cls->members = ems_list_prepend(cls->members, member);

    return EMS_OK;
}

EMSMessageClassMember *_ems_message_type_get_member(EMSMessageClassInternal *cls, const char *member_name)
{
    EMSList *tmp;
    for (tmp = cls->members; tmp; tmp = tmp->next) {
        if (!strcmp(((EMSMessageClassMember *)tmp->data)->name, member_name))
            return (EMSMessageClassMember *)tmp->data;
    }
    return NULL;
}

void _ems_message_class_internal_free(EMSMessageClassInternal *msgclass)
{
    ems_list_free_full(msgclass->members, (EMSDestroyNotifyFunc)ems_free);
}

void ems_message_types_clear(void)
{
    ems_list_free_full(msg_classes, (EMSDestroyNotifyFunc)_ems_message_class_internal_free);
    msg_classes = NULL;
}

void ems_messages_set_magic(char *magic)
{
    strncpy(msg_magic, magic, 4);
}

/* Create a new message of the given type, followed by the recipient and sender ids.
 * After this a “NULL, NULL”-terminated sequence of key/value-pairs may follow.
 */
EMSMessage *ems_message_new(uint32_t type, uint64_t recipient_id, uint64_t sender_id, ...)
{
    EMSMessageClassInternal *cls = _ems_message_type_get_class(type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *msg = ems_alloc0(cls->klass.size);
    msg->type = type;
    msg->recipient_id = recipient_id;
    msg->sender_id = sender_id;

    va_list args;
    va_start(args, sender_id);

    char *key;
    EMSMessageClassMember *member;

    while ((key = va_arg(args, char *)) != NULL) {
        member =  _ems_message_type_get_member(cls, key);
        if (ems_unlikely(!member)) {
            fprintf(stderr, "Message has no member with the name `%s'\n", key);
            (void)va_arg(args, void *);
        }
        else if (member->set_cb) {
            member->set_cb(msg,
                           member->id,
                           (void *)(((void *)msg) + member->offset),
                           va_arg(args, void *));
        }
        else {
            switch (member->type) {
                case EMS_MSG_MEMBER_INT:
                    *((int *)(((void *)msg) + member->offset)) = va_arg(args, int);
                    break;
                case EMS_MSG_MEMBER_INT64:
                    *((int64_t *)(((void *)msg) + member->offset)) = va_arg(args, int64_t);
                    break;
                case EMS_MSG_MEMBER_UINT:
                    *((uint32_t *)(((void *)msg) + member->offset)) = va_arg(args, uint32_t);
                    break;
                case EMS_MSG_MEMBER_UINT64:
                    *((uint64_t *)(((void *)msg) + member->offset)) = va_arg(args, uint64_t);
                    break;
                case EMS_MSG_MEMBER_DOUBLE:
                    *((double *)(((void *)msg) + member->offset)) = va_arg(args, double);
                    break;
                case EMS_MSG_MEMBER_POINTER:
                    *((void **)(((void *)msg) + member->offset)) = va_arg(args, void *);
                    break;
                case EMS_MSG_MEMBER_FIXED_STRING:
                    strcpy((char *)(((void *)msg) + member->offset), va_arg(args, char *));
                    break;
                case EMS_MSG_MEMBER_STRING:
                    {
                        char *value = va_arg(args, char *);
                        size_t len = value ? strlen(value) : 0;
                                                ems_free(*(char **)(((void *)msg) + member->offset));

                        *((char **)(((void *)msg) + member->offset)) = ems_alloc(len + 1);

                        if (value)
                            strncpy(*(char **)(((void *)msg) + member->offset), value, len + 1);
                        else
                            (*((char **)(((void *)msg) + member->offset)))[0] = 0;
                    }
                    break;
                case EMS_MSG_MEMBER_ARRAY:
                    ems_array_copy(((EMSArray *)(((void *)msg) + member->offset)),
                                   (EMSArray *)va_arg(args, void *));
                    break;
                default:
                    fprintf(stderr, "Unable to set value of type %d in member `%s'\n", member->type, key);
                    (void)va_arg(args, void *);
                    break;
            }
        }
    }

    va_end(args);

    return msg;
}

/* Encode a message. This calls the function from the class or writes only the generic part. */
size_t ems_message_encode(EMSMessage *msg, uint8_t **buffer)
{
    if (!msg)
        return 0;
    EMSMessageClassInternal *cls = _ems_message_type_get_class(msg->type);
    if (ems_unlikely(!cls))
        return 0;

    size_t buflen = EMS_MESSAGE_HEADER_SIZE + cls->klass.min_payload;
    *buffer = ems_alloc(buflen);

    strncpy((char *)(*buffer), msg_magic, 4);
    ems_message_write_u32(*buffer, 4, msg->type);
    ems_message_write_u64(*buffer, 8, msg->recipient_id);
    ems_message_write_u64(*buffer, 16, msg->sender_id);
    ems_message_write_u32(*buffer, 24, 0);

    if (cls->klass.msg_encode)
        return cls->klass.msg_encode(msg, buffer, buflen);

    return buflen;
}

/* Decode a message. */
void ems_message_decode_payload(EMSMessage *msg, uint8_t *payload, size_t payload_size)
{
    EMSMessageClassInternal *cls = _ems_message_type_get_class(msg->type);

    if (cls->klass.msg_decode)
        cls->klass.msg_decode(msg, payload, payload_size);
}

EMSMessage *ems_message_decode_header(uint8_t *buffer, size_t buflen, size_t *payload_size)
{
    if (buflen < EMS_MESSAGE_HEADER_SIZE || !buffer)
        return NULL;

    if (strncmp((char *)buffer, msg_magic, 4))
        return NULL;

    uint32_t type = ems_message_read_u32(buffer, 4);
    EMSMessageClassInternal *cls = _ems_message_type_get_class(type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *msg = ems_alloc(cls->klass.size);
    msg->type = type;
    msg->recipient_id = ems_message_read_u64(buffer, 8);
    msg->sender_id = ems_message_read_u64(buffer, 16);

    if (payload_size)
        *payload_size = (size_t)ems_message_read_u32(buffer, 24);

    return msg;
}

/* Free a message. */
void ems_message_free(EMSMessage *msg)
{
    EMSMessageClassInternal *cls;
    if (msg) {
        cls = _ems_message_type_get_class(msg->type);
        if (cls && cls->klass.msg_free)
            cls->klass.msg_free(msg);
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

    EMSMessageClassInternal *cls = _ems_message_type_get_class(src->type);
    if (cls && cls->klass.msg_copy)
        cls->klass.msg_copy(dst, src);

    return EMS_OK;
}

/* Duplicate a message. */
EMSMessage *ems_message_dup(EMSMessage *msg)
{
    if (ems_unlikely(!msg))
        return NULL;

    EMSMessageClassInternal *cls = _ems_message_type_get_class(msg->type);
    if (ems_unlikely(!cls))
        return NULL;

    EMSMessage *new_msg = ems_alloc0(cls->klass.size);
    new_msg->type = msg->type;

    ems_message_copy(new_msg, msg);

    return new_msg;
}
