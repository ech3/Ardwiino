/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (CFG_TUH_ENABLED && CFG_TUH_XINPUT)
#include "defines.h"
#include "descriptors.h"
#include "hid.h"
#include "host/usbh.h"
#include "host/usbh_classdriver.h"
#include "xinput_host.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t type;
    uint8_t subtype;

    uint16_t epin_size;
    uint16_t epout_size;

    uint8_t epin_buf[CFG_TUH_XINPUT_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUH_XINPUT_EPOUT_BUFSIZE];
} xinputh_interface_t;

typedef struct
{
    uint8_t inst_count;
    xinputh_interface_t instances[CFG_TUH_XINPUT];
} xinputh_device_t;
static xinputh_device_t _xinputh_dev[CFG_TUH_DEVICE_MAX];

//------------- Internal prototypes -------------//

// Get HID device & interface
TU_ATTR_ALWAYS_INLINE static inline xinputh_device_t *get_dev(uint8_t dev_addr);
TU_ATTR_ALWAYS_INLINE static inline xinputh_interface_t *get_instance(uint8_t dev_addr, uint8_t instance);
static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf);
static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr);

//--------------------------------------------------------------------+
// Interface API
//--------------------------------------------------------------------+

uint8_t tuh_xinput_instance_count(uint8_t dev_addr) {
    return get_dev(dev_addr)->inst_count;
}

bool tuh_xinput_mounted(uint8_t dev_addr, uint8_t instance) {
    if (get_dev(dev_addr)->inst_count < instance) return false;
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    return (hid_itf->ep_in != 0) || (hid_itf->ep_out != 0);
}

//--------------------------------------------------------------------+
// Interrupt Endpoint API
//--------------------------------------------------------------------+

bool tuh_xinput_receive_report(uint8_t dev_addr, uint8_t instance) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);

    // claim endpoint
    TU_VERIFY(usbh_edpt_claim(dev_addr, hid_itf->ep_in));

    if (!usbh_edpt_xfer(dev_addr, hid_itf->ep_in, hid_itf->epin_buf, hid_itf->epin_size)) {
        usbh_edpt_release(dev_addr, hid_itf->ep_in);
        return false;
    }

    return true;
}

bool tuh_xinput_ready(uint8_t dev_addr, uint8_t instance) {
    TU_VERIFY(tuh_xinput_mounted(dev_addr, instance));

    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    return !usbh_edpt_busy(dev_addr, hid_itf->ep_in);
}

bool tuh_xinput_send_report(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    // claim endpoint
    TU_VERIFY(usbh_edpt_claim(dev_addr, hid_itf->ep_out));
    memcpy(hid_itf->epout_buf, report, len);
    if (!usbh_edpt_xfer(dev_addr, hid_itf->ep_out, hid_itf->epout_buf, len)) {
        usbh_edpt_release(dev_addr, hid_itf->ep_out);
        return false;
    }
    return true;
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+
void xinputh_init(void) {
    tu_memclr(_xinputh_dev, sizeof(_xinputh_dev));
}

bool xinputh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    (void)result;

    uint8_t const dir = tu_edpt_dir(ep_addr);
    uint8_t const instance = get_instance_id_by_epaddr(dev_addr, ep_addr);
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);

    if (dir == TUSB_DIR_IN) {
        tuh_xinput_report_received_cb(dev_addr, instance, hid_itf->epin_buf, (uint16_t)xferred_bytes);
        usbh_edpt_xfer(dev_addr, hid_itf->ep_in, hid_itf->epin_buf, hid_itf->epin_size);
    }

    return true;
}

void xinputh_close(uint8_t dev_addr) {
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );

    xinputh_device_t *hid_dev = get_dev(dev_addr);

    if (tuh_xinput_umount_cb) {
        for (uint8_t inst = 0; inst < hid_dev->inst_count; inst++) tuh_xinput_umount_cb(dev_addr, inst);
    }

    tu_memclr(hid_dev, sizeof(xinputh_device_t));
}

//--------------------------------------------------------------------+
// Enumeration
//--------------------------------------------------------------------+

bool xinputh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    (void)rhport;
    (void)max_len;
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass || TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);
    xinputh_interface_t *p_xinput = NULL;
    for (uint8_t i = 0; i < CFG_TUH_XINPUT; i++) {
        xinputh_interface_t *hid_itf = get_instance(dev_addr, i);
        if (hid_itf->ep_in == 0 && hid_itf->ep_out == 0) {
            p_xinput = hid_itf;
            break;
        }
    }
    TU_VERIFY(p_xinput, 0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
    // Support standard HID devices
    if (TUSB_CLASS_HID == desc_itf->bInterfaceClass) {
        TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);

        uint8_t const *p_desc = (uint8_t const *)desc_itf;
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        USB_HID_DESCRIPTOR *x_desc =
            (USB_HID_DESCRIPTOR *)p_desc;
        TU_ASSERT(HID_DESCRIPTOR_HID == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;

        p_xinput->type = UNKNOWN;
        uint8_t temp_buf[512];
        uint8_t *current = temp_buf;
        uint8_t len = x_desc->wDescriptorLength;
        uint8_t last_id = 0;
        tuh_descriptor_get_hid_report(dev_addr, p_xinput->itf_num, x_desc->bDescrType, 0, temp_buf, x_desc->wDescriptorLength, NULL, (uintptr_t)temp_buf);
        // Seems that sometimes we miss the first byte?
        if (!temp_buf[0]) {
            temp_buf[0] = 0x05;
        }
        // Extremely simple hid report parser, need to walk down and find feature reports to detect generic PS4 controllers.
        // TODO: if at some point we want to support hid controllers, we can parse out the report more here, and actually work out a mapping
        while (len) {
            // Size is first two bits
            uint8_t size = (current[0] & 0b11) + 1;
            uint8_t type = current[0] >> 2 & 0b11;
            uint8_t tag = current[0] >> 4;
            if (type == HID_REPORT_TYPE_GLOBAL && tag == HID_REPORT_TAG_GLOBAL_REPORT_ID) {
                last_id = current[1];
            }
            // PS4 controllers define a feature request of 0x0303
            // .... except the DS4 or knockoffs of it, but we end up doing a vid pid lookup for those anyways
            if (type == HID_REPORT_TYPE_MAIN && tag == HID_REPORT_TAG_MAIN_FEATURE && last_id == 0x03) {
                p_xinput->type = PS4;
            }
            len -= size;
            current += size;
            if (!size) {
                break;
            }
        }
        _xinputh_dev->inst_count++;
        return true;
    }

    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    if (desc_itf->bInterfaceSubClass == 0x5D &&
        (desc_itf->bInterfaceProtocol == 0x01 ||
         desc_itf->bInterfaceProtocol == 0x03 ||
         desc_itf->bInterfaceProtocol == 0x02)) {
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        p_xinput->type = XBOX360;
        if (desc_itf->bInterfaceProtocol == 0x01) {
            p_xinput->subtype = x_desc->subtype;
            usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
        }
        _xinputh_dev->inst_count++;
        return true;

    } else if (desc_itf->bInterfaceSubClass == 0xfD &&
               desc_itf->bInterfaceProtocol == 0x13) {
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        p_desc = tu_desc_next(p_desc);
        p_xinput->type = XBOX360;
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        _xinputh_dev->inst_count++;
        return true;
    } else if (desc_itf->bInterfaceSubClass == 0x47 &&
               desc_itf->bInterfaceProtocol == 0xD0 && desc_itf->bNumEndpoints) {
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        p_xinput->type = XBOXONE;
        _xinputh_dev->inst_count++;
        usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
        return true;
    }
    return false;
}

//--------------------------------------------------------------------+
// Set Configure
//--------------------------------------------------------------------+

enum {
    CONFG_SET_IDLE,
    CONFIG_SET_PROTOCOL,
    CONFIG_GET_REPORT_DESC,
    CONFIG_COMPLETE
};

static void config_driver_mount_complete(uint8_t dev_addr, uint8_t instance);
static void process_set_config(tuh_xfer_t *xfer);

bool xinputh_set_config(uint8_t dev_addr, uint8_t itf_num) {
    uint8_t instance = get_instance_id_by_itfnum(dev_addr, itf_num);
    config_driver_mount_complete(dev_addr, instance);
    return true;
}

static void config_driver_mount_complete(uint8_t dev_addr, uint8_t instance) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    // enumeration is complete
    tuh_xinput_mount_cb(dev_addr, instance, hid_itf->type, hid_itf->subtype);

    // notify usbh that driver enumeration is complete
    usbh_driver_set_config_complete(dev_addr, hid_itf->itf_num);
}

//--------------------------------------------------------------------+
// Helper
//--------------------------------------------------------------------+

// Get Device by address
TU_ATTR_ALWAYS_INLINE static inline xinputh_device_t *get_dev(uint8_t dev_addr) {
    return &_xinputh_dev[dev_addr - 1];
}

// Get Interface by instance number
TU_ATTR_ALWAYS_INLINE static inline xinputh_interface_t *get_instance(uint8_t dev_addr, uint8_t instance) {
    return &_xinputh_dev[dev_addr - 1].instances[instance];
}

// Get instance ID by interface number
static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf) {
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++) {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((hid->itf_num == itf)) return inst;
    }

    return 0xff;
}

// Get instance ID by endpoint address
static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr) {
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++) {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((ep_addr == hid->ep_in) || (ep_addr == hid->ep_out)) return inst;
    }

    return 0xff;
}

#endif
