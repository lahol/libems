#pragma once

#include "ems-error.h"
#include "ems-message.h"
#include "ems-peer.h"
#include "ems-status-messages.h"

/* Setup internal data structures.
 * magic is the 4 byte magic sequence every message starts with.
 */
int ems_init(char *magic);

/* Clean up internal data structures. */
void ems_cleanup(void);
