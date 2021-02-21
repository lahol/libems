/** @file ems.h
 *  Main include for the library.
 *  @defgroup libems Setup the library.
 *  @{
 */
#pragma once

#include "ems-error.h"
#include "ems-message.h"
#include "ems-peer.h"
#include "ems-status-messages.h"
#include "ems-memory.h"

/** @brief Setup internal data structures.
 *  @param[in] magic The 4 byte magic sequence every message starts with.
 *  @retval 0 The initialization was successful.
 *  @retval <0 An error code.
 */
int ems_init(char *magic);

/** @brief Clean up internal data structures.
 */
void ems_cleanup(void);

/** @} */
