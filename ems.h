#pragma once

#include "ems-error.h"
#include "ems-message.h"

/* Setup internal data structures. (generate peers, communicators?) */
int ems_init(char *magic);

/* Clean up internal data structures. */
void ems_cleanup(void);
