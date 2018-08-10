#include "ems-message.h"
#include "ems-memory.h"
#include <memory.h>

EMSMessage *ems_message_new(uint32_t payload_size)
{
    EMSMessage *msg = ems_alloc(sizeof(EMSMessage) + payload_size);
    memset(msg, 0, sizeof(EMSMessage));
    msg->payload_size = payload_size;
    msg->payload = ((uint8_t *)msg) + sizeof(EMSMessage);

    return msg;
}

void ems_message_free(EMSMessage *msg)
{
    if (msg) {
        ems_free(msg->payload);
        ems_free(msg);
    }
}
