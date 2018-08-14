#pragma once

/* Everything is fine. */
#define EMS_OK                                          0

/* We try to register a message type that already exists. */
#define EMS_ERROR_MESSAGE_TYPE_EXISTS                   1

/* The message types do not match, e.g., for copying. */
#define EMS_ERROR_MESSAGE_TYPE_MISMATCH                 2
