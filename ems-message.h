#pragma once

#include <stdint.h>
#include <arpa/inet.h>
#include <ems-types.h>

#define EMS_MESSAGE_RECIPIENT_MASTER 0
#define EMS_MESSAGE_RECIPIENT_ALL    ((uint64_t)0xffffffffffffffff)

/* Some (arbitrary) offset for user-defined messages. We need some non-internal messages
 * to propagate changes ourselves. Keep in mind that we might change this according
 * to ems-status-messages.h. */
#define EMS_MESSAGE_USER                   0x00000010

typedef struct {
    uint32_t type;           /* The application-defined message type. */
    uint64_t recipient_id;   /* The identifier of the recipient or (uint32_t)(-1) for all. */
    uint64_t sender_id;      /* The identifier of the sender. */
} EMSMessage;

/* In the binary stream, the generic message header consists of the following:
 * 4 byte: magic string, indicating the start of a message used with this library
 * 4 byte: type, uint32_t in network byte order
 * 8 byte: recipient_id
 * 8 byte: sender_id
 * 4 byte: payload size
 */
#define EMS_MESSAGE_HEADER_SIZE 28 /* magic + the above + payload_size*/

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

/* Available member types
 */
typedef enum {
    EMS_MSG_MEMBER_UNKNOWN = EMS_TYPE_UNKNOWN,
    EMS_MSG_MEMBER_UINT = EMS_TYPE_UINT,
    EMS_MSG_MEMBER_INT = EMS_TYPE_INT,
    EMS_MSG_MEMBER_UINT64 = EMS_TYPE_UINT64,
    EMS_MSG_MEMBER_INT64 = EMS_TYPE_INT64,
    EMS_MSG_MEMBER_DOUBLE = EMS_TYPE_DOUBLE,
    EMS_MSG_MEMBER_POINTER = EMS_TYPE_POINTER,
    EMS_MSG_MEMBER_FIXED_STRING = EMS_TYPE_FIXED_STRING,
    EMS_MSG_MEMBER_STRING = EMS_TYPE_STRING,
    EMS_MSG_MEMBER_CUSTOM
} EMSMessageMemberType;

/* Callback to set a member of a message.
 * EMSMessage *: pointer to the message
 * uint32_t:     identifier given during registration
 * void *:       a pointer to the member to be set
 * void *:       a pointer to the value given
 */
typedef int (*EMSMessageClassSetMemberCallback)(EMSMessage *, uint32_t, void *, void *);

/* Add a member to the class.
 * msgtype:       The type of the message class this member is added to.
 * member_type:   The type of the member.
 * member_id:     User-defined identifier. This may be useful for callbacks.
 * member_name:   Unique name of the member.
 * member_offset: The offset of the member in the corresponding struct, as determined by offsetof().
 * member_set_cb: If not NULL, use this callback to set the value.
 */
int ems_message_type_add_member(uint32_t msgtype,
                                EMSMessageMemberType member_type,
                                uint32_t member_id,
                                const char *member_name,
                                size_t member_offset,
                                EMSMessageClassSetMemberCallback member_set_cb);

/* Clear the classes list. */
void ems_message_types_clear(void);

/* Set the magic 4 byte string sent as the first 4 bytes in the message header of each message. */
void ems_messages_set_magic(char *magic);

/* Create a new message of the given type, followed by the recipient and sender ids.
 * After this a “NULL, NULL”-terminated sequence of key/value-pairs may follow.
 */
EMSMessage *ems_message_new(uint32_t type, uint64_t recipient_id, uint64_t sender_id, ...);

/* Copy a message. */
int ems_message_copy(EMSMessage *dst, EMSMessage *src);

/* Duplicate a message. */
EMSMessage *ems_message_dup(EMSMessage *msg);

#if 0
/* Set a value given by some key. */
void ems_message_set_value(EMSMessage *msg, const char *key, const void *value);
#endif

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

/* Check whether this is an internal message. All internal messages
 * have the high bit set. Other messages must not use this bit.
 */
#define EMS_MESSAGE_IS_INTERNAL(msg) ((msg) && ((msg)->type & 0x80000000))

/* The following functions provide means to read/write integers to or from the
 * byte stream payload. While not required – we do not care for the payload –
 * those are strongly recommended, since they will take care of the byte order
 * for you.
 */

/* Write a 32 bit value to the payload at a given offset. */
static inline void ems_message_write_u32(uint8_t *payload, uint32_t offset, uint32_t value)
{
    value = htonl(value);
    payload[offset    ] = value & 0xff;
    payload[offset + 1] = (value >>  8) & 0xff;
    payload[offset + 2] = (value >> 16) & 0xff;
    payload[offset + 3] = (value >> 24) & 0xff;
}

/* Write a 64 bit value to the payload at a given offset. */
static inline void ems_message_write_u64(uint8_t *payload, uint32_t offset, uint64_t value)
{
    uint32_t vl = (uint32_t)(value & 0xffffffff);
    uint32_t vh = (uint32_t)(value >> 32);
    ems_message_write_u32(payload, offset, vl);
    ems_message_write_u32(payload, offset + 4, vh);
}

/* Write the payload size of the message. */
static inline void ems_message_write_payload_size(uint8_t *msgbuffer, uint32_t payload_size)
{
    ems_message_write_u32(msgbuffer, EMS_MESSAGE_HEADER_SIZE - 4, payload_size);
}

/* Read a 32 bit value from the payload at a given offset. */
static inline uint32_t ems_message_read_u32(uint8_t *payload, uint32_t offset)
{
    uint32_t value;
    value = ((payload[offset + 3] & 0xff) << 24) |
            ((payload[offset + 2] & 0xff) << 16) |
            ((payload[offset + 1] & 0xff) << 8) |
            (payload[offset] & 0xff);

    return ntohl(value);
}

/* Read a 64 bit value from the payload at a given offset. */
static inline uint64_t ems_message_read_u64(uint8_t *payload, uint32_t offset)
{
    uint32_t vl;
    uint32_t vh;
    vl = ems_message_read_u32(payload, offset);
    vh = ems_message_read_u32(payload, offset + 4);

    return (((uint64_t)vh) << 32) | (((uint64_t)vl) & 0x00000000ffffffff);
}
