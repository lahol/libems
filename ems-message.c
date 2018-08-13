#include "ems-message.h"
#include "ems-memory.h"
#include "ems-util.h"
#include <memory.h>

EMSMessage *ems_message_new(uint32_t payload_size)
{
    EMSMessage *msg = ems_alloc(sizeof(EMSMessage) + payload_size);
    memset(msg, 0, sizeof(EMSMessage));
    msg->payload_size = payload_size;
    msg->payload = ((uint8_t *)msg) + sizeof(EMSMessage);

    return msg;
}

EMSMessage *ems_message_dup(EMSMessage *msg)
{
    if (ems_unlikely(!msg))
        return NULL;

    EMSMessage *dup = ems_alloc(sizeof(EMSMessage) + msg->payload_size);

    *dup = *msg;

    dup->payload = ((uint8_t *)dup) + sizeof(EMSMessage);
    memcpy(dup->payload, msg->payload, msg->payload_size);

    return dup;
}

void ems_message_free(EMSMessage *msg)
{
    if (msg) {
        ems_free(msg);
    }
}

EMSMessage *ems_message_create(uint32_t type,
                               uint64_t uuid,
                               uint32_t part,
                               uint32_t sender_id,
                               uint32_t recipient_id,
                               uint32_t payload_size,
                               uint8_t *payload)
{
    EMSMessage *msg = ems_message_new(payload_size);

    msg->type = type;
    msg->uuid = uuid;
    msg->part = part;
    msg->sender_id = sender_id;
    msg->recipient_id = recipient_id;

    memcpy(msg->payload, payload, payload_size);

    return msg;
}

/* buffer must be at least EMS_MESSAGE_HEADER_SIZE + msg->payload_size */
void ems_message_to_net(uint8_t *buffer, EMSMessage *msg)
{
    strcpy((char *)buffer, "EMSG");
    ems_message_write_u32(buffer, 4, msg->type);
    ems_message_write_u64(buffer, 8, msg->uuid);
    ems_message_write_u32(buffer, 16, msg->part);
    ems_message_write_u32(buffer, 20, msg->sender_id);
    ems_message_write_u32(buffer, 24, msg->recipient_id);
    ems_message_write_u32(buffer, 28, msg->payload_size);
    memcpy(&buffer[32], msg->payload, msg->payload_size);
}

EMSMessage *ems_message_from_net(uint8_t *buffer, uint32_t length)
{
    EMSMessage *msg;
    if (length < EMS_MESSAGE_HEADER_SIZE)
        return NULL;
    if (strncmp((char *)buffer, "EMSG", 4))
        return NULL;
    uint32_t pl_size = ems_message_read_u32(buffer, 28);
    if (length < EMS_MESSAGE_HEADER_SIZE + pl_size)
        return NULL;
    msg = ems_message_new(pl_size);

    msg->type = ems_message_read_u32(buffer, 4);
    msg->uuid = ems_message_read_u64(buffer, 8);
    msg->part = ems_message_read_u32(buffer, 16);
    msg->sender_id = ems_message_read_u32(buffer, 20);
    msg->recipient_id = ems_message_read_u32(buffer, 24);

    memcpy(msg->payload, &buffer[32], msg->payload_size);

    return msg;
}
