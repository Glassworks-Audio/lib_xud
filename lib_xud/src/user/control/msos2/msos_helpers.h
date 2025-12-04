// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* MSOS 2.0 Descriptors for vendor Endpoint 0 handling */

#ifndef _MSOS_HELPERS_H_
#define _MSOS_HELPERS_H_

#include <stddef.h>
#include <stdint.h>

#include "xud.h"
#include "xud_device.h"

#if defined(__XC__) || defined(__cplusplus)
extern "C" {
#endif

#if !defined(__XC__)
/* These types and functions are only available to C code. Due to struct packing requirements in the MSOS descriptors, they cannot be used in XC code.
 * If the endpoint0 code is run in XC, then a small wrapper should be used to initialise the BOS and MSOS descriptors.
 * For single interface example, see `simple_ep0_msos_descriptors.h` */

#include "msos_descriptors.h"

/** Descriptor handle
 * 
 * Used to pass descriptor pointer and size. As the type will change with different applications, as will the length.
 * The USB driver will only want a byte array and length.
 */
typedef struct desc_handle_t
{
    unsigned char *desc_ptr;
    size_t desc_size;
} desc_handle_t;

/** Register the application MSOS and BOS descriptors, before enumeration.
 * 
 * \param bos_descs    Pointer to the BOS descriptor handle
 * \param msos_descs   Pointer to the MSOS descriptor handle
 */
void XUD_RegisterMsosDescriptors(const desc_handle_t *bos_descs, const desc_handle_t *msos_descs);

/** Update the device interface GUID in the MSOS descriptor, before enumeration.
 * 
 * The device MSOS 2.0 GUID is used by the host to bind the correct driver to the device.
 * The example GUID provided are intended to use with WINUSB driver on Windows.
 * 
 * When user's develop their own USB device, they should generate their own unique GUID for the device
 * interface and link this to the host driver.
 * 
 * \param registry  Pointer to the registry property descriptor to update
 * \param guid_str  String containing the device interface GUID for the control interface.
 *                  Must be in the format "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" and 
 *                  DEVICE_INTERFACE_GUID_MAX_STRLEN long, plus null terminator.
 */
void XUD_Update_Guid_In_Msos_Desc(MSOS_desc_registry_property_t *registry, const char *guid_str);

#endif

/** Endpoint0 function to send the BOS descriptor when prompted via a Standard Get request
 * 
 * Request will be Standard Get request (USB_GET_DESCRIPTOR) with wValue high byte == USB_DESCTYPE_BOS
 * 
 * \param ep0_out   Endpoint 0 OUT endpoint
 * \param ep0_in    Endpoint 0 IN endpoint
 * \param sp        Pointer to the setup packet of the request
 * 
 * \retval          XUD_RES_ERR if request not handled
 * \retval          XUD_RES_OKAY if successful
 * \retval          XUD_RES_WAIT if transfer in progress
 */
XUD_Result_t XUD_GetBosDescriptor(XUD_ep ep0_out, XUD_ep ep0_in, const USB_SetupPacket_t *sp);

/** Endpoint0 function to send the MSOS descriptor when prompted via a Vendor Get request
 * 
 * Request will be a Vendor Get request with bRequest == XUD_REQUEST_GET_MSOS_DESCRIPTOR.
 * This is defined in xud.h
 * 
 * \param ep0_out   Endpoint 0 OUT endpoint
 * \param ep0_in    Endpoint 0 IN endpoint
 * \param sp        Pointer to the setup packet of the request
 * 
 * \retval          XUD_RES_ERR if request not handled
 * \retval          XUD_RES_OKAY if successful
 * \retval          XUD_RES_WAIT if transfer in progress
 */
XUD_Result_t XUD_GetMsosDescriptor(XUD_ep ep0_out, XUD_ep ep0_in, const USB_SetupPacket_t *sp);

#if defined(__XC__) || defined(__cplusplus)
} // extern "C"
#endif

#endif
