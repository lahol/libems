#pragma once

#include <stdint.h>
#include <arpa/inet.h>

#define EMS_MESSAGE_RECIPIENT_MASTER 0
#define EMS_MESSAGE_RECIPIENT_ALL    0xffffffff

typedef struct {
    uint32_t type;           /* The application-defined message type. */
    uint32_t recipient_id;   /* The identifier of the recipient or (uint32_t)(-1) for all. */
    uint32_t sender_id;      /* The identifier of the sender. */
} EMSMessage;

/* In the binary stream, the generic message header consists of the following:
 * 4 byte: magic string, indicating the start of a message used with this library
 * 4 byte: type, uint32_t in network byte order
 * 4 byte: recipient_id
 * 4 byte: sender_id
 * 4 byte: payload size
 */
#define EMS_MESSAGE_HEADER_SIZE 20 /* magic + the above */

typedef struct {
    /* The type of the message belonging to this class. */
    uint32_t msgtype;

    /* The size of the message structure belonging to this class. */
    size_t size;

    /* The minimal expected payload size. This will be used to reduce the
     * possible realloc a little. */
    size_t min_payload;

    /* Encode the message for sending over the network.
     * msg, pointer receiving the new buffer, [in] initial length of buffer
     * Returns the length of the new buffer, leave the first EMS_MESSAGE_HEADER_SIZE bytes untouched.
     */
    size_t (*msg_encode)(EMSMessage *, uint8_t **, size_t);

    /* Decode the message from the network.
     * msg, payload, length of buffer
     */
    void (*msg_decode)(EMSMessage *, uint8_t *, size_t);

    /* Free the message. If this is NULL, ems_free() is used. */
    void (*msg_free)(EMSMessage *);

    /* Set a value in the message.
     * msg, key, value
     */
    void (*msg_set_value)(EMSMessage *, const char *, const void *);

    /* Copy the message. */
    void (*msg_copy)(EMSMessage *, EMSMessage *);
} EMSMessageClass;

/* Register a new message type. The type id shall be a user definded constant, since we want
 * to use this by multiple, heterogenous hosts. Therefore, we cannot return the type id from
 * an internal register.
 * Return an error if the type is already there. Internal messages all have the high bit set,
 * so avoiding those ids, there should be no type clash.
 * The user has to provide the functions for encoding/decoding the message as well as
 * setting values by key.
 */
int ems_message_register_type(uint32_t type, EMSMessageClass *msg_class);

/* Clear the classes list. */
void ems_message_types_clear(void);

/* Set the magic 4 byte string sent as the first 4 bytes in the message header of each message. */
void ems_messages_set_magic(char *magic);

/* Create a new message of the given type, followed by the recipient and sender ids.
 * After this a “NULL, NULL”-terminated sequence of key/value-pairs may follow.
 */
EMSMessage *ems_message_new(uint32_t type, uint32_t recipient_id, uint32_t sender_id, ...);

/* Copy a message. */
int ems_message_copy(EMSMessage *dst, EMSMessage *src);

/* Duplicate a message. */
EMSMessage *ems_message_dup(EMSMessage *msg);

/* Set a value given by some key. */
void ems_message_set_value(EMSMessage *msg, const char *key, const void *value);

/* Encode a message. This calls the function from the class or writes only the generic part. */
size_t ems_message_encode(EMSMessage *msg, uint8_t **buffer);

/* Decode a message. */
void ems_message_decode_payload(EMSMessage *msg, uint8_t *payload, size_t payload_size);

/* Only decode the payload size. This is used to read the rest of the message. */
EMSMessage *ems_message_decode_header(uint8_t *buffer, size_t buflen, size_t *payload_size);

/* Free a message. */
void ems_message_free(EMSMessage *msg);

/* Message, userdata */
typedef void (*EMSHandleMessage)(EMSMessage *, void *);

#define EMS_MESSAGE_IS_INTERNAL(msg) ((msg) && ((msg)->type & 0x80000000))

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
