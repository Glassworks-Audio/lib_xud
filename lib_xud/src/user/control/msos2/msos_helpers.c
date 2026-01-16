// Copyright 2025-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* MSOS 2.0 Descriptors for vendor Endpoint 0 handling */

#include "msos_helpers.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "msos_descriptors.h"
#include "xud_device.h"

static desc_handle_t bos_handle = { NULL, 0, };

static desc_handle_t msos_handle = { NULL, 0, };

void XUD_RegisterMsosDescriptors(const desc_handle_t *bos_descs, const desc_handle_t *msos_descs)
{
    if (bos_descs != NULL) {
        bos_handle.desc_ptr = bos_descs->desc_ptr;
        bos_handle.desc_size = bos_descs->desc_size;
    }

    if (msos_descs != NULL) {
        msos_handle.desc_ptr = msos_descs->desc_ptr;
        msos_handle.desc_size = msos_descs->desc_size;
    }
}

void XUD_Update_Guid_In_Msos_Desc(MSOS_desc_registry_property_t *registry, const char *guid_str)
{
    if (guid_str == NULL || registry == NULL) {
        return;
    }
    unsigned char *msos_guid_ptr = registry->PropertyData;
    size_t guid_len = strnlen(guid_str, DEVICE_INTERFACE_GUID_MAX_STRLEN + 1);

    if (guid_len != DEVICE_INTERFACE_GUID_MAX_STRLEN) {
        return;
    }

    // Convert char array to UTF-16LE
    for(int i = 0; i < DEVICE_INTERFACE_GUID_MAX_STRLEN; i++)
    {
        msos_guid_ptr[2 * i] = guid_str[i];
        msos_guid_ptr[(2 * i) + 1] = 0x0;
    }
}

XUD_Result_t XUD_GetBosDescriptor(XUD_ep ep0_out, XUD_ep ep0_in, const USB_SetupPacket_t *sp)
{
    XUD_Result_t result = XUD_RES_ERR;

    if ((bos_handle.desc_ptr != NULL) && ((sp->wValue & 0xff00) == (USB_DESCTYPE_BOS << 8))) {
        result = XUD_DoGetRequest(ep0_out, ep0_in, bos_handle.desc_ptr, bos_handle.desc_size, sp->wLength);
    }
    return result;
}

XUD_Result_t XUD_GetMsosDescriptor(XUD_ep ep0_out, XUD_ep ep0_in, const USB_SetupPacket_t *sp)
{
    XUD_Result_t result = XUD_RES_ERR;

    if ((msos_handle.desc_ptr != NULL) && (sp->wIndex == MS_OS_20_DESCRIPTOR_INDEX)) {
        result = XUD_DoGetRequest(ep0_out, ep0_in, msos_handle.desc_ptr, msos_handle.desc_size, sp->wLength);
    }
    return result;
}
