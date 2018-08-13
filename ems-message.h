#pragma once

#include <stdint.h>
#include <arpa/inet.h>

#define EMS_MESSAGE_RECIPIENT_MASTER 0
#define EMS_MESSAGE_RECIPIENT_ALL    0xffffffff

typedef struct {
    uint32_t type;           /* The application-defined message type. */
    uint64_t uuid;           /* An identifier of a message for multi-part messages. */
    uint32_t part;           /* The part of the multi-part message. */
    uint32_t sender_id;      /* The identifier of the sender. */
    uint32_t recipient_id;   /* The identifier of the recipient or (uint32_t)(-1) for all. */
    uint32_t payload_size;   /* The length of payload. */
    uint8_t *payload;        /* The actual message. */
} EMSMessage;

#define EMS_MESSAGE_HEADER_SIZE 32 /* magic + the above */

/* Message, userdata */
typedef void (*EMSHandleMessage)(EMSMessage *, void *);

EMSMessage *ems_message_new(uint32_t payload_size);
EMSMessage *ems_message_dup(EMSMessage *msg);
void ems_message_free(EMSMessage *msg);

EMSMessage *ems_message_create(uint32_t type,
                               uint64_t uuid,
                               uint32_t part,
                               uint32_t sender_id,
                               uint32_t recipient_id,
                               uint32_t payload_size,
                               uint8_t *payload);

void ems_message_to_net(uint8_t *buffer, EMSMessage *msg);
EMSMessage *ems_message_from_net(uint8_t *buffer, uint32_t length);


static inline void ems_message_write_u32(uint8_t *payload, uint32_t offset, uint32_t value)
{
    value = htonl(value);
    payload[offset    ] = value & 0xff;
    payload[offset + 1] = (value >>  8) & 0xff;
    payload[offset + 2] = (value >> 16) & 0xff;
    payload[offset + 3] = (value >> 24) & 0xff;
}

static inline void ems_message_write_u64(uint8_t *payload, uint32_t offset, uint64_t value)
{
    uint32_t vl = (uint32_t)(value & 0xffffffff);
    uint32_t vh = (uint32_t)(value >> 32);
    ems_message_write_u32(payload, offset, vl);
    ems_message_write_u32(payload, offset + 4, vh);
}

static inline uint32_t ems_message_read_u32(uint8_t *payload, uint32_t offset)
{
    uint32_t value;
    value = ((payload[offset + 3] & 0xff) << 24) |
            ((payload[offset + 2] & 0xff) << 16) |
            ((payload[offset + 1] & 0xff) << 8) |
            (payload[offset] & 0xff);

    return ntohl(value);
}

static inline uint64_t ems_message_read_u64(uint8_t *payload, uint32_t offset)
{
    uint32_t vl;
    uint32_t vh;
    vl = ems_message_read_u32(payload, offset);
    vh = ems_message_read_u32(payload, offset + 4);

    return (((uint64_t)vh) << 32) | (((uint64_t)vl) & 0x00000000ffffffff);
}

/* Internal messages have the high bit set to 1. */
/* When we accepted a new slave, inform it about its id. */
#define EMS_MESSAGE_TYPE_SET_ID 0x80000001

/* Either the master or the slave is about to leave */
#define EMS_MESSAGE_TYPE_LEAVE  0x80000002
