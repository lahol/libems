/* Initialize or cleanup the library. */
#include "ems.h"
#include "ems-messages-internal.h"
#include "ems-error.h"

/* Setup internal data structures. (generate peers, communicators?) */
int ems_init(char *magic)
{
    int rc;
    if ((rc = ems_messages_register_internal_types()) != EMS_OK)
        return rc;

    ems_messages_set_magic(magic ? magic : "EMSG");

    return EMS_OK;
}

/* Clean up internal data structures. */
void ems_cleanup(void)
{
    ems_message_types_clear();
}
