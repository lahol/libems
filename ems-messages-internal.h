/* Internal messages used for inter-peer communication
 * the user has nothing to do with.
 * All internal messages will have the high bit set to 1, which
 * the user-defined messages are not allowed to.
 */
#pragma once

#include "ems-message.h"

/* Internal messages have the high bit set to 1. */
/* When we accepted a new slave, inform it about its id. */
#define __EMS_MESSAGE_SET_ID   0x80000001
typedef struct {
    EMSMessage parent;

    uint32_t peer_id;
} EMSMessageIntSetId;

/* Either the master or the slave is about to leave */
#define __EMS_MESSAGE_LEAVE    0x80000002
typedef EMSMessage EMSMessageIntLeave;

/* Terminate the whole group */
#define __EMS_MESSAGE_TERM     0x80000003
typedef EMSMessage EMSMessageIntTerm;

/* Acknowledge the termination request */
#define __EMS_MESSAGE_TERM_ACK 0x80000004
typedef EMSMessage EMSMessageIntTermAck;

/* New connection */
#define __EMS_MESSAGE_CONNECTION_ADD 0x80000005
typedef EMSMessage EMSMessageIntConnectionAdd;

/* Removed connection */
#define __EMS_MESSAGE_CONNECTION_DEL 0x80000006
typedef EMSMessage EMSMessageIntConnectionDel;

/* Register those internal types. This gets called once from ems_init. */
int ems_messages_register_internal_types(void);
