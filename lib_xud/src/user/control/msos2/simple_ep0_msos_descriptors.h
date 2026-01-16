// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* MSOS 2.0 Descriptors for vendor Endpoint 0 handling */

#ifndef _SIMPLE_EP0_MSOS_DESCRIPTORS_H_
#define _SIMPLE_EP0_MSOS_DESCRIPTORS_H_

#include <stddef.h>
#include <stdint.h>

#include "xud.h"
#include "xud_device.h"

/* Example of simple, single interface, MSOS 2.0 descriptor
 * 
 * For more complex examples, see lib_xua DFU and Control in xua_ep0_msos_descriptors.h
 */

#if defined(__XC__) || defined(__cplusplus)
extern "C" {
#endif

/** Initialise the Simple Ep0 MSOS Descriptors before enumeration of the device */
void XUD_Init_Simple_Ep0_Msos_Descriptors(void);

#if defined(__XC__) || defined(__cplusplus)
} // extern "C"
#endif

#endif
