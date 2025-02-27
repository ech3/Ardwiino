#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>
#include <tusb.h>

#include "bt.h"
#include "commands.h"
#include "common/tusb_types.h"
#include "config.h"
#include "controllers.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "hardware/watchdog.h"
#include "hidescriptorparser.h"
#include "host/usbh_classdriver.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pins.h"
#include "reports/controller_reports.h"
#include "serial.h"
#include "shared_main.h"
#include "xinput_device.h"
#include "xinput_host.h"
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN STRING_DESCRIPTOR_PICO serialstring = {
    .bLength = (sizeof(uint8_t) + sizeof(uint8_t) + SERIAL_LEN),
    .bDescriptorType = USB_DESCRIPTOR_STRING,
    .UnicodeString = {}};

static uint32_t __uninitialized_ram(persistedConsoleType);
static uint32_t __uninitialized_ram(persistedConsoleTypeValid);
uint8_t xone_dev_addr = 0;
uint8_t x360_dev_addr = 0;
uint8_t ps4_dev_addr = 0;
bool connected = false;

uint8_t total_usb_host_devices = 0;
typedef struct {
    USB_Device_Type_t type;
    USB_LastReport_Data_t report;
    uint8_t report_length;
} Usb_Host_Device_t;

Usb_Host_Device_t usb_host_devices[CFG_TUH_DEVICE_MAX];
PS3_REPORT ble_rx_report;
volatile bool ble_rx_report_received = false;
typedef struct {
    uint8_t pin_dp;
    uint8_t pio_tx_num;
    uint8_t sm_tx;
    uint8_t tx_ch;
    uint8_t pio_rx_num;
    uint8_t sm_rx;
    uint8_t sm_eop;
    void *alarm_pool;
    int8_t debug_pin_rx;
    int8_t debug_pin_eop;
} pio_usb_configuration_t;

bool ready_for_next_packet() {
    return tud_xinput_n_ready(0) && tud_ready_for_packet();
}

bool usb_connected() {
    return connected;
}

void send_report_to_pc(const void *report, uint8_t len) {
    tud_xinput_n_report(0, 0, report, len);
}
void loop() {
    tick();
    tud_task();
#if USB_HOST_STACK
    tuh_task();
#endif
}
#ifdef BLUETOOTH_RX
bool bluetooth_report_available(void) {
    return ble_rx_report_received;
}
void bluetooth_report_read(void* buf) {
    ble_rx_report_received = false;
    memcpy(buf, &ble_rx_report, sizeof(ble_rx_report));
}
void bluetooth_report_set(const void* buf) {
    ble_rx_report_received = true;
    memcpy(&ble_rx_report, ((uint8_t*)buf)+1, sizeof(ble_rx_report));
}
#endif
void setup() {
    generateSerialString(&serialstring);
#if USB_HOST_STACK
    pio_usb_configuration_t config = {
        USB_HOST_DP_PIN, 0, 0, 0, 1, 0, 1, NULL, -1, -1};
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);
#endif
    tusb_init();
    if (persistedConsoleTypeValid == 0x3A2F) {
        consoleType = persistedConsoleType;
    }
    printf("ConsoleType: %d\r\n", consoleType);
    init_main();
#if BLUETOOTH
    btstack_main();
#endif
}
uint8_t get_device_address_for(uint8_t deviceType) {
    if (deviceType == XBOXONE) {
        return xone_dev_addr;
    }
    if (deviceType == XBOX360) {
        return x360_dev_addr;
    }
    if (deviceType == PS4) {
        return ps4_dev_addr;
    }
    return 0;
}

void send_report_to_controller(uint8_t dev_addr, uint8_t *report, uint8_t len) {
    if (dev_addr && tuh_xinput_mounted(dev_addr, 0)) {
        tuh_xinput_send_report(dev_addr, 0, report, len);
    }
}
void tud_mount_cb(void) {
    device_reset();
    connected = true;
}
uint8_t get_usb_host_device_count() {
    return total_usb_host_devices;
}

USB_Device_Type_t get_usb_host_device_type(uint8_t id) {
    return usb_host_devices[id].type;
}

void get_usb_host_device_data(uint8_t id, uint8_t *buf) {
    memcpy(buf, &usb_host_devices[id].report, usb_host_devices[id].report_length);
}

uint8_t read_usb_host_devices(uint8_t *buf) {
    for (int i = 0; i < total_usb_host_devices; i++) {
        USB_Device_Type_t *type = &usb_host_devices[i].type;
        buf[(i * 3)] = type->console_type;
        buf[(i * 3) + 1] = type->sub_type;
        buf[(i * 3) + 2] = type->rhythm_type;
    }
    return total_usb_host_devices * 3;
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {
    printf("Detected controller: %d (%d) on %d\r\n", controllerType, subtype, dev_addr);
    uint16_t host_vid = 0;
    uint16_t host_pid = 0;
    tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
    USB_Device_Type_t type = get_usb_device_type_for(host_vid, host_pid);
    type.dev_addr = dev_addr;
    if (controllerType == XBOX360) {
        if (subtype) {
            type.console_type = controllerType;
            switch (subtype) {
                case XINPUT_GUITAR:
                case XINPUT_GUITAR_ALTERNATE:
                    type.sub_type = GUITAR;
                    break;

                case XINPUT_DRUMS:
                    type.sub_type = DRUMS;
                    break;

                case XINPUT_STAGE_KIT:
                    type.sub_type = STAGE_KIT;
                    break;

                case XINPUT_TURNTABLE:
                    type.sub_type = DJ_HERO_TURNTABLE;
                    break;

                default:
                    type.sub_type = GAMEPAD;
                    break;
            }

            x360_dev_addr = dev_addr;
            xinput_controller_connected(host_vid, host_pid, subtype);
            usb_host_devices[total_usb_host_devices].type = type;
            total_usb_host_devices++;
        }
    } else if (controllerType == XBOXONE) {
        xone_dev_addr = dev_addr;
        type.console_type = XBOXONE;
        xone_controller_connected(dev_addr);
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;

    } else if (controllerType == UNKNOWN) {
        if (type.console_type) {
            usb_host_devices[total_usb_host_devices].type = type;
            total_usb_host_devices++;
        }
    } else if (controllerType == PS4) {
        type.console_type = PS4;
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        if (!ps4_dev_addr) {
            ps4_dev_addr = dev_addr;

            printf("Found PS4 controller\r\n");
            ps4_controller_connected(dev_addr, host_vid, host_pid);
        }
    }
    printf("Total devices: %d\r\n", total_usb_host_devices);
    
    host_controller_connected();
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    if (xone_dev_addr == dev_addr) {
        xone_dev_addr = 0;
    }
    if (x360_dev_addr == dev_addr) {
        x360_dev_addr = 0;
    }
    if (ps4_dev_addr == dev_addr) {
        ps4_dev_addr = 0;
        ps4_controller_disconnected();
    }
    // Probably should actulaly work out what was unplugged and all that
    total_usb_host_devices = 0;
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
    return buf;
}
uint8_t const *tud_hid_custom_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(USB_DESCRIPTOR_CONFIGURATION << 8, 0, buf);
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialstring;
    }
    if (descriptorRequest(USB_DESCRIPTOR_STRING << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (dev_addr == xone_dev_addr) {
        receive_report_from_controller(report, len);
    }
    for (int i = 0; i < total_usb_host_devices; i++) {
        if (usb_host_devices[i].type.dev_addr == dev_addr) {
            if (usb_host_devices[i].type.console_type == XBOXONE) {
                GipHeader_t *header = (GipHeader_t *)report;
                if (header->command != GHL_HID_REPORT && header->command != GIP_INPUT_REPORT) {
                    return;
                }
            }
            memcpy(&usb_host_devices[i].report, report, len);
            usb_host_devices[i].report_length = len;
            return;
        }
    }
}

uint8_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer) {
    if (!dev_addr) {
        // Device is not connected yet!
        return 0;
    }
    tusb_control_request_t setup = {
        bmRequestType : requestType,
        bRequest : request,
        wValue : wValue,
        wIndex : wIndex,
        wLength : wLength
    };
    tuh_xfer_t xfer = {};
    xfer.daddr = dev_addr;
    xfer.ep_addr = 0;
    xfer.setup = &setup;
    xfer.buffer = buffer;
    xfer.complete_cb = NULL;
    xfer.user_data = 0;
    tuh_control_xfer(&xfer);
    if (xfer.result != XFER_RESULT_SUCCESS) {
        return false;
    }
    return xfer.actual_len;
}
tusb_control_request_t lastreq;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD && request->bRequest == TUSB_REQ_GET_DESCRIPTOR) {
        //------------- STD Request -------------//
        if (stage == CONTROL_STAGE_SETUP) {
            uint8_t const desc_type = tu_u16_high(request->wValue);
            // uint8_t const desc_index = tu_u16_low (request->wValue);

            if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_HID) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_HID << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_REPORT) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else {
                return false;  // stall unsupported request
            }
        }
    }
    if (controlRequestValid(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength)) {
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            if (stage == CONTROL_STAGE_SETUP) {
                uint16_t len = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
                tud_control_xfer(rhport, request, buf, len);
            }
        } else {
            if (stage == CONTROL_STAGE_SETUP) {
                tud_control_xfer(rhport, request, buf, request->wLength);
            }
            if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
                controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
            }
        }
    }

    return true;
}

usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device_HID",
#endif
        .init = xinputd_init,
        .reset = xinputd_reset,
        .open = xinputd_open,
        .control_xfer_cb = tud_vendor_control_xfer_cb,
        .xfer_cb = xinputd_xfer_cb,
        .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver;
}

usbh_class_driver_t driver_host[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Host_HID",
#endif
        .init = xinputh_init,
        .open = xinputh_open,
        .set_config = xinputh_set_config,
        .xfer_cb = xinputh_xfer_cb,
        .close = xinputh_close}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver_host;
}
void reboot(void) {
    watchdog_enable(1, false);
    for (;;) {
    }
}
void bootloader(void) {
    reset_usb_boot(0, 0);
}
void reset_usb(void) {
    persistedConsoleType = consoleType;
    persistedConsoleTypeValid = 0x3A2F;
    reboot();
}