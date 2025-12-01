// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* MSOS 2.0 Descriptors for vendor Endpoint 0 handling */

#include "simple_ep0_msos_descriptors.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "msos_descriptors.h"
#include "msos_helpers.h"
#include "xud.h"

/** Example of simple, single interface, MSOS 2.0 descriptor
 * 
 * For more complex examples, see lib_xua DFU and Control in xua_ep0_msos_descriptors.h
 */
typedef struct {
  MSOS_desc_header_t              msos_desc_header;
  MSOS_desc_compat_id_t           msos_desc_compat_id;
  MSOS_desc_registry_property_t   msos_desc_registry_property;
} __attribute__((packed)) MSOS_desc_simple_t;

/* USB Binary Device Object Store (BOS) descriptor */
static USB_Descriptor_BOS_t desc_bos_control =
{
    .usb_desc_bos_standard = {
        .bLength = sizeof(USB_Descriptor_BOS_standard_t),
        .bDescriptorType = USB_DESCTYPE_BOS,
        .wTotalLength = sizeof(USB_Descriptor_BOS_standard_t) + sizeof(USB_Descriptor_BOS_platform_t),
        .bNumDeviceCaps = 1
    },
    .usb_desc_bos_platform = {
        .bLength = sizeof(USB_Descriptor_BOS_platform_t),
        .bDescriptorType = USB_DESCTYPE_DEVICE_CAPABILITY,
        .bDevCapabilityType = DEVICE_CAPABILITY_PLATFORM,
        .bReserved = 0,
        .PlatformCapabilityUUID = {USB_BOS_MS_OS_20_UUID},
        .CapabilityData = {U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(sizeof(MSOS_desc_simple_t)), XUD_REQUEST_GET_MSOS_DESCRIPTOR, 0}
    }
};

/* USB MSOS 2.0 descriptor */
static MSOS_desc_simple_t desc_ms_os_20_simple =
{
    .msos_desc_header =
    {
        .wLength = sizeof(MSOS_desc_header_t),
        .wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR,
        .dwWindowsVersion = 0x06030000, // 0603 == Windows 8.1
        .wTotalLength = sizeof(MSOS_desc_simple_t)
    },
    .msos_desc_compat_id =
    {
        .wLength = sizeof(MSOS_desc_compat_id_t),
        .wDescriptorType = MS_OS_20_FEATURE_COMPATIBLE_ID,
        .CompatibleID = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
        .SubCompatibleID = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    },
    .msos_desc_registry_property =
    {
        .wLength = sizeof(MSOS_desc_registry_property_t),
        .wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY,
        .wPropertyDataType = MSOS_REG_PROPERTY_DATA_TYPE_REG_MULTI_SZ,
        .wPropertyNameLength = MSOS_PROPERTY_NAME_LEN,
        .PropertyName = {'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
                         'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00}, //"DeviceInterfaceGUIDs\0" in UTF-16
        .wPropertyDataLength = MSOS_INTERFACE_GUID_LEN,
        // "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\0\0" generate from https://guidgenerator.com/ and stored as Unicode
        .PropertyData = { 0 } // defined in the XUD_WINUSB_DEVICE_INTERFACE_GUID_CONTROL define and updated in the descriptor at runtime with update_guid_in_msos_desc()
    }
};


/* Device Interface GUID */
static char g_device_interface_guid_control_str[DEVICE_INTERFACE_GUID_MAX_STRLEN + 1] = XUD_WINUSB_DEVICE_INTERFACE_GUID_CONTROL;

void XUD_Init_Simple_Ep0_Msos_Descriptors(void)
{
    // Apply valid GUID
    if (strnlen(g_device_interface_guid_control_str, (DEVICE_INTERFACE_GUID_MAX_STRLEN + 1)) == DEVICE_INTERFACE_GUID_MAX_STRLEN) {
        XUD_Update_Guid_In_Msos_Desc(&desc_ms_os_20_simple.msos_desc_registry_property, g_device_interface_guid_control_str);
    }

    desc_handle_t bos_handle = {
        (unsigned char*)&desc_bos_control, sizeof(USB_Descriptor_BOS_t),
    };

    desc_handle_t msos_handle = {
        (unsigned char*)&desc_ms_os_20_simple, sizeof(MSOS_desc_simple_t),
    };
    XUD_RegisterMsosDescriptors(&bos_handle, &msos_handle);
}
