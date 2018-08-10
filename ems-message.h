#pragma once

#include <stdint.h>

typedef struct {
    uint32_t type;           /* The application-defined message type. */
    uint64_t uuid;           /* An identifier of a message for multi-part messages. */
    uint32_t part;           /* The part of the multi-part message. */
    uint32_t sender_id;      /* The identifier of the sender. */
    uint32_t payload_size;   /* The length of payload. */
    uint8_t *payload;        /* The actual message. */
} EMSMessage;

/* Message, userdata */
typedef void (*EMSHandleMessage)(EMSMessage *, void *);

EMSMessage *ems_message_new(uint32_t payload_size);
void ems_message_free(EMSMessage *msg);
