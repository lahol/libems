#pragma once

/* Everything is fine. */
#define EMS_OK                                          0

/* We try to register a message type that already exists. */
#define EMS_ERROR_MESSAGE_TYPE_EXISTS                   1

/* The message types do not match, e.g., for copying. */
#define EMS_ERROR_MESSAGE_TYPE_MISMATCH                 2

/* The initialization failed for some reason. */
#define EMS_ERROR_INITIALIZATION                        3

/* We have an invalid socket. */
#define EMS_ERROR_INVALID_SOCKET                        4

/* Some connection error */
#define EMS_ERROR_CONNECTION                            5

/* Invalid arguments */
#define EMS_ERROR_INVALID_ARGUMENT                      6

/* A required class function was not set */
#define EMS_ERROR_MISSING_CLASS_FUNCTION                7
