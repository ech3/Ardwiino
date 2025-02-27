#pragma once
#include <stdint.h>

// Types of requests used between the 8u2/16u2 and the 328p/mega2560
#define READY 0x65
#define USB_READY 0x64
#define VALID_PACKET 0x14
#define CONTROL_REQUEST_VALIDATION_ID 0x71
#define CONTROL_REQUEST_INVALID_ID 0x72
#define CONTROLLER_DATA_RESTART_USB_ID 0x73
#define DESCRIPTOR_SERIAL_ID 0x74
#define CONTROLLER_DATA_TRANSMIT_ID 0x75
#define CONTROLLER_DATA_REQUEST_ID 0x76
#define DESCRIPTOR_ID 0x77
#define CONTROL_REQUEST_ID 0x78
#define DEVICE_ID 0x79

// Baudrate used for communication between 8u2/16u2 and the 328p/mega2560 when in controller mode
#define BAUD 500000
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)

// States used for the 8u2/16u2 packet receive state machines
#define STATE_NO_PACKET 0
#define STATE_READ_PACKET_TYPE 1
#define STATE_READ_LENGTH 2
#define STATE_READ_VALID 3
#define STATE_READ_DEVICE_ID 4
#define STATE_READ_AND_RETURN 5

// structure of data sent between 8u2/16u2 and the 328p/mega2560 when in controller mode
typedef struct {
    uint8_t magic;
    uint8_t id;
    uint8_t len;
} __attribute__((packed)) packet_header_t;
typedef struct {
    packet_header_t header;
    uint8_t state;
} __attribute__((packed)) usb_ready_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} __attribute__((packed)) data_transmit_packet_t;

typedef struct {
    packet_header_t header;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) descriptor_request_t;

typedef struct {
    packet_header_t header;
    uint8_t bmRequestType;
    uint8_t request;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    uint8_t data[];
} __attribute__((packed)) control_request_t;