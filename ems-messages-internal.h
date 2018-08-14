#pragma once

#include "ems-message.h"

/* Internal messages have the high bit set to 1. */
/* When we accepted a new slave, inform it about its id. */
#define __EMS_MESSAGE_TYPE_SET_ID 0x80000001
typedef struct {
    EMSMessage parent;

    uint32_t peer_id;
} EMSMessageIntSetId;

/* Either the master or the slave is about to leave */
#define __EMS_MESSAGE_TYPE_LEAVE  0x80000002
typedef struct {
    EMSMessage parent;
} EMSMessageIntLeave;

int ems_messages_register_internal_types(void);
