#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pio_keepalive_high.pio.h"
#include "pio_keepalive_low.pio.h"
#include "pio_serialiser.pio.h"
#include "utils.h"

#define readLen 2048

struct repeating_timer timer;
uint8_t buffer2[readLen] = {0};
uint8_t buffer3[readLen / 8] = {};
uint8_t bufferka[100];
uint8_t packetbufferka[100];
int dma_chan_read;
int dma_chan_write;
int dma_chan_keepalive;
uint sm;
PIO pio;
uint offset;
uint sm_keepalive;
uint offset_keepalive;
uint32_t currentFrame = 0;
uint32_t keepalive_delay = 0;
#define T_ATTR_PACKED __attribute__((packed))
typedef struct T_ATTR_PACKED {
    union {
        struct T_ATTR_PACKED {
            uint8_t recipient : 5;  ///< Recipient type tusb_request_recipient_t.
            uint8_t type : 2;       ///< Request type tusb_request_type_t.
            uint8_t direction : 1;  ///< Direction type. tusb_dir_t
        } bmRequestType_bit;

        uint8_t bmRequestType;
    };

    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} tusb_control_request_t;
typedef enum {
    TUSB_DIR_OUT = 0,
    TUSB_DIR_IN = 1,

    TUSB_DIR_IN_MASK = 0x80
} tusb_dir_t;

#define SETUP 0b10110100
#define DATA0 0b11000011
#define DATA1 0b11010010
#define OUT 0b10000111
#define IN 0b10010110
#define NAK 0b01011010
#define ACK 0b01001011
#define SOF 0b10100101

#define USB_DP_PIN 20
#define USB_DM_PIN 21
#define USB_FIRST_PIN 20
float div;
bool full_speed = false;
typedef struct {
    uint id;
    uint id_crc;
    uint oneCount;
    uint8_t buffer[100];
    uint8_t bufferCRC[100];
} state_t;
state_t state_ka;
state_t state_pk;
static void reset_state(state_t* state) {
    state->id_crc = 0;
    state->id = 0;
    memset(state->buffer, 0, 4);
    memset(state->bufferCRC, 0, sizeof(state->bufferCRC));
}
bool lastJ = false;
static void writeBit(uint8_t dm, uint8_t dp, state_t* state) {
    // Give us the ability to swap wires if needed
    if (USB_FIRST_PIN == USB_DM_PIN) {
        state->buffer[state->id >> 3] |= dp << (state->id & 7);
        state->id++;
        state->buffer[state->id >> 3] |= dm << (state->id & 7);
        state->id++;
    } else {
        state->buffer[state->id >> 3] |= dm << (state->id & 7);
        state->id++;
        state->buffer[state->id >> 3] |= dp << (state->id & 7);
        state->id++;
    }
    // >>3 is equiv to /8 (2^3), %8 is equiv to & 7 (8-1)
}
static void J(state_t* state) {
    lastJ = true;
    // for full_speed=true, J is DM low, DP high
    // for full_speed=false, J is DM low, DP high
    writeBit(full_speed, !full_speed, state);
}
static void K(state_t* state) {
    lastJ = false;
    // for full_speed=true, K is DM high, DP low
    // for full_speed=false, K is DM high, DP low
    writeBit(!full_speed, full_speed, state);
}
static void SE0(state_t* state) {
    writeBit(0, 0, state);
}
static void sync(state_t* state) {
    K(state);
    J(state);
    K(state);
    J(state);
    K(state);
    J(state);
    K(state);
    K(state);
}
static void EOP(state_t* state) {
    SE0(state);
    SE0(state);
    J(state);
}
void sendData(uint16_t byte, int count, bool reverse, state_t* state) {
    for (int i = 0; i < count; i++) {
        // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
        // 1 bit is transmitted by leaving the data lines as-is.
        if (!bit_check(byte, reverse ? (count - 1 - i) : i)) {
            // Zero
            if (lastJ) {
                K(state);
            } else {
                J(state);
            }
            state->oneCount = 0;
        } else {
            bit_set(state->bufferCRC[state->id_crc / 8], state->id_crc % 8);
            // One
            if (lastJ) {
                J(state);
            } else {
                K(state);
            }
            state->oneCount++;
            // Bit stuffing - if more than 6 one bits are set, then send an extra 0 bit
            if (state->oneCount == 7) {
                // Toggle lines
                if (lastJ) {
                    K(state);
                } else {
                    J(state);
                }
                state->oneCount = 0;
            }
        }
        state->id_crc++;
    }
}
void sendByte(uint8_t byte, state_t* state) {
    sendData(byte, 8, false, state);
}
void sendNibble(uint8_t byte, state_t* state) {
    sendData(byte, 4, false, state);
}

void sendAddress(uint8_t byte, state_t* state) {
    sendData(byte, 7, false, state);
}
void sendPID(uint8_t byte, state_t* state) {
    sendData(byte, 8, true, state);
}

void printReceivedP() {
    for (int i = 0; i < readLen / 8 / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer3[i]));
    }
    printf("\n");
    for (int i = 0; i < readLen / 8 / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf("0x%x, ", buffer3[i]);
    }
    printf("\n");
}
void printReceived() {
    for (int i = 0; i < readLen / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(buffer2[i]));
    }
    printf("\n");
}

uint16_t crc_update(const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
    unsigned int i;
    bool bit;
    unsigned char c;
    uint16_t crc = 0xffff;
    while (data_len--) {
        c = *d++;
        for (i = 0x01; i & 0xff; i <<= 1) {
            bit = crc & 0x8000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x8005;
            }
        }
        crc &= 0xffff;
    }

    uint16_t ret = crc & 0x01;
    for (i = 1; i < 16; i++) {
        crc >>= 1;
        ret = (ret << 1) | (crc & 0x01);
    }
    return ret ^ 0xffff;
}

void sendCRC16(state_t* state) {
    sendData(crc_update(state->bufferCRC, state->id_crc / 8), 16, false, state);
}
void sendCRC5(state_t* state) {
    uint8_t byte;
    if (state->id_crc == 11) {
        byte = crc5_usb_11bit_input(state->bufferCRC);
    } else {
        byte = crc5_usb_19bit_input(state->bufferCRC);
    }
    sendData(byte, 5, false, state);
}
void WR(bool wait, state_t* state) {
    // Now that we know what we are writing, left pad so we can write to the 32 bit fifo buffer
    uint8_t bufferTmp[100];
    int tmpId = state->id;
    memcpy(bufferTmp, state->buffer, (tmpId / 8) + 1);
    reset_state(state);
    int remaining = (32 - (tmpId % 32)) / 2;
    for (int i = 0; i < remaining; i++) {
        J(state);
    }
    int currentBit = 0;
    while (currentBit != tmpId) {
        bit_write(bit_check(bufferTmp[currentBit / 8], (currentBit % 8)), state->buffer[state->id / 8], (state->id % 8));
        currentBit++;
        state->id++;
    }
    printf("About to write:\n");
    printf("bits: %d\n", state->id);
    printf("transfers: %d\n", state->id / 32);
    for (int i = 0; i < state->id / 8; i++) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf(PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(state->buffer[i]));
    }

    memset(buffer2, 0, sizeof(buffer2));
    memset(buffer3, 0, sizeof(buffer3));

    pio_serialiser_program_init(pio, sm, offset, USB_FIRST_PIN, div);
    dma_channel_set_read_addr(dma_chan_write, state->buffer, false);
    dma_channel_set_trans_count(dma_chan_write, state->id / 32, true);
    dma_channel_set_write_addr(dma_chan_read, buffer2, false);
    dma_channel_set_trans_count(dma_chan_read, readLen / 32, true);
    dma_channel_wait_for_finish_blocking(dma_chan_read);
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_set_consecutive_pindirs(pio, sm_keepalive, USB_FIRST_PIN, 2, true);
    int syncCount = 0;
    bool readingData = false;
    bool valid = true;
    bool foundSomething = false;
    bool lastWasJ;
    bool skip = false;
    // printReceived();
    uint8_t oneCount;
    uint8_t bit = 0;
    for (int i = 0; i < readLen; i += 2) {
        uint8_t bit1 = !!bit_check(buffer2[i / 8], i % 8);
        uint8_t bit2 = !!bit_check(buffer2[i / 8], (i + 1) % 8);
        uint8_t dm, dp;
        if (USB_FIRST_PIN == USB_DM_PIN) {
            dm = bit1;
            dp = bit2;
        } else {
            dp = bit1;
            dm = bit2;
        }
        // DM = 1 DP = 0 = J for full_speed
        // for full_speed=true, J is DM low, DP high
        // for full_speed=false, J is DM low, DP high
        bool j = dm == !full_speed && dp == full_speed;
        bool k = dm == full_speed && dp == !full_speed;
        bool se0 = !dm && !dp;

        if (k) {
            printf("K");
        } else if (j) {
            printf("J");
        } else if (se0) {
            printf("SE0");
        }
        if (readingData) {
            if (se0) {
                readingData = false;
                printf("DONE\n");
                break;
            }
            // 0 bit is transmitted by toggling the data lines from J to K or vice versa.
            // 1 bit is transmitted by leaving the data lines as-is.
            if (lastWasJ != j) {
                if (!skip) {
                    bit_clear(buffer3[bit / 8], bit % 8);
                    bit++;
                }
                skip = false;
                oneCount = 0;
            } else {
                bit_set(buffer3[bit / 8], bit % 8);
                bit++;
                skip = false;
                oneCount++;
                if (oneCount == 7) {
                    skip = true;
                    oneCount = 0;
                }
            }
        } else {
            if (j) {
                // J
                switch (syncCount) {
                    case 1:
                    case 3:
                    case 5:
                        syncCount++;
                        break;
                    default:
                        valid = false;
                        break;
                }
            } else if (k) {
                // K
                switch (syncCount) {
                    case 0:
                    case 2:
                    case 4:
                    case 6:
                        syncCount++;
                        break;
                    case 7:
                        readingData = true;
                        break;
                    default:
                        valid = false;
                        break;
                }
            }
        }
        lastWasJ = j;
    }
    printf("\n");

    // printReceivedP();
    reset_state(state);
}

void dma_handler() {
    dma_hw->ints0 = 1u << dma_chan_keepalive;
    // Increment current frame and then append its crc5
    currentFrame++;
    if (currentFrame > (1 << 11) - 1) {
        currentFrame = 0;
    }
    reset_state(&state_ka);
    sendData(currentFrame++, 11, false, &state_ka);
    sendCRC5(&state_ka);
    dma_channel_start(dma_chan_keepalive);
}

void initKeepAlive() {
    keepalive_delay = (clock_get_hz(clk_sys) / div / 4 / 1000) - (((full_speed ? 32 : 0) + 3));
    if (full_speed) {
        offset_keepalive = pio_add_program(pio, &pio_keepalive_high_program);
        pio_keepalive_high_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
        // we need to DMA a frame number and a CRC
        dma_handler();
    } else {
        offset_keepalive = pio_add_program(pio, &pio_keepalive_low_program);
        pio_keepalive_low_program_init(pio, sm_keepalive, offset_keepalive, USB_FIRST_PIN, div, keepalive_delay);
    }
}
void sendSetup(uint8_t address, uint8_t endpoint, tusb_control_request_t request, uint8_t* d) {
    printf("Token\n");
    sync(&state_pk);
    sendPID(SETUP, &state_pk);
    memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
    state_pk.id_crc = 0;
    sendAddress(address, &state_pk);
    sendNibble(endpoint, &state_pk);
    sendCRC5(&state_pk);
    EOP(&state_pk);
    J(&state_pk);
    J(&state_pk);
    sync(&state_pk);
    sendPID(DATA0, &state_pk);
    memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
    state_pk.id_crc = 0;
    uint8_t* data = (uint8_t*)&request;
    for (int i = 0; i < sizeof(tusb_control_request_t); i++) {
        sendByte(*data++, &state_pk);
    }
    sendCRC16(&state_pk);
    EOP(&state_pk);
    WR(true, &state_pk);
    printf("Direction: %d\n", request.bmRequestType_bit.direction);
    if (request.bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if (request.wLength) {
            printf("Data\n");
            sync(&state_pk);
            sendPID(OUT, &state_pk);
            memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
            state_pk.id_crc = 0;
            sendAddress(address, &state_pk);
            sendNibble(endpoint, &state_pk);
            sendCRC5(&state_pk);
            EOP(&state_pk);
            sync(&state_pk);
            sendPID(DATA1, &state_pk);
            memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
            state_pk.id_crc = 0;
            uint8_t* data = (uint8_t*)&d;
            for (int i = 0; i < request.wLength; i++) {
                sendByte(*data++, &state_pk);
            }
            sendCRC16(&state_pk);
            EOP(&state_pk);
            WR(false, &state_pk);
        }
        printf("Status\n");
        sync(&state_pk);
        sendPID(IN, &state_pk);
        memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
        state_pk.id_crc = 0;
        sendAddress(address, &state_pk);
        sendNibble(endpoint, &state_pk);
        sendCRC5(&state_pk);
        EOP(&state_pk);
        WR(false, &state_pk);

        printf("ACK\n");
        sync(&state_pk);
        sendPID(ACK, &state_pk);
        EOP(&state_pk);
        WR(false, &state_pk);
        printf("Done\n");
    } else {
        if (request.wLength) {
            sync(&state_pk);
            sendPID(IN, &state_pk);
            memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
            state_pk.id_crc = 0;
            sendAddress(address, &state_pk);
            sendNibble(endpoint, &state_pk);
            sendCRC5(&state_pk);
            EOP(&state_pk);
            WR(true, &state_pk);
        }
        printf("Data\n");
        sync(&state_pk);
        sendPID(OUT, &state_pk);
        memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
        state_pk.id_crc = 0;
        sendAddress(address, &state_pk);
        sendNibble(endpoint, &state_pk);
        sendCRC5(&state_pk);
        EOP(&state_pk);

        sync(&state_pk);
        sendPID(DATA0, &state_pk);
        memset(state_pk.bufferCRC, 0, sizeof(state_pk.bufferCRC));
        state_pk.id_crc = 0;
        sendCRC16(&state_pk);
        EOP(&state_pk);
        WR(true, &state_pk);
        printf("Status\n");
    }
}

void initPIO() {
    pio = pio0;
    pio = pio1;

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    sm = pio_claim_unused_sm(pio, true);
    sm_keepalive = pio_claim_unused_sm(pio, true);

    offset = pio_add_program(pio, &pio_serialiser_program);

    dma_chan_write = dma_claim_unused_channel(true);
    dma_chan_read = dma_claim_unused_channel(true);
    dma_chan_keepalive = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_chan_write);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_write_increment(&c, false);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));

    dma_channel_configure(
        dma_chan_write,
        &c,
        &pio->txf[sm],  // Write address (only need to set this once)
        state_pk.buffer,
        0,
        false);

    dma_channel_config cr = dma_channel_get_default_config(dma_chan_read);
    channel_config_set_transfer_data_size(&cr, DMA_SIZE_32);
    channel_config_set_write_increment(&cr, true);
    channel_config_set_read_increment(&cr, false);
    channel_config_set_dreq(&cr, pio_get_dreq(pio, sm, false));
    dma_channel_configure(
        dma_chan_read,
        &cr,
        buffer2,  // Write address (only need to set this once)
        &pio->rxf[sm],
        readLen / 32,
        false);
    if (full_speed) {
        dma_channel_config cka = dma_channel_get_default_config(dma_chan_keepalive);
        channel_config_set_transfer_data_size(&cka, DMA_SIZE_32);
        channel_config_set_write_increment(&cka, false);
        channel_config_set_read_increment(&cka, false);
        channel_config_set_dreq(&cka, pio_get_dreq(pio, sm_keepalive, true));
        dma_channel_set_irq0_enabled(dma_chan_keepalive, true);
        irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
        irq_set_enabled(DMA_IRQ_0, true);
        dma_channel_configure(
            dma_chan_keepalive,
            &cka,
            &pio->txf[sm_keepalive],  // Write address (only need to set this once)
            state_pk.buffer,
            1,  //Just write 32 bits
            false);
    }
}
/*------------- MAIN -------------*/
int main(void) {
    // If its needed, 120mhz would give us perfectly even clock dividers.
    set_sys_clock_khz(120000, true);
    stdio_init_all();
    gpio_init(USB_DM_PIN);
    gpio_init(USB_DP_PIN);
    while (1) {
        // D- pull up = Low, D+ pull up = full
        if (gpio_get(USB_DM_PIN)) {
            break;
        } else if (gpio_get(USB_DP_PIN)) {
            break;
        }
    }
    sleep_ms(2);
    while (1) {
        // D- pull up = Low, D+ pull up = full
        // Could we set the processor clock so it evenly divides into usb
        if (gpio_get(USB_DM_PIN)) {
            div = (clock_get_hz(clk_sys) / ((1500000.0f))) / 4;
            full_speed = false;
            break;
        } else if (gpio_get(USB_DP_PIN)) {
            div = (clock_get_hz(clk_sys) / ((12000000.0f))) / 4;
            full_speed = true;
            break;
        }
    }
    printf("Speed: %d %f %d\n", full_speed, div, clock_get_hz(clk_sys));
    initPIO();
    // Reset
    sleep_ms(1000);
    pio_sm_set_pins(pio, sm, 0);
    sleep_ms(100);
    pio_sm_set_pins(pio, sm, _BV(USB_DM_PIN));
    initKeepAlive();
    sleep_ms(1000);
    uint8_t received[100];
    tusb_control_request_t req = {.bmRequestType = {0x00}, .bRequest = 0x05, .wValue = 0x0C};
    sendSetup(0x00, 0x00, req, received);
    // // printReceived();
    // // printReceivedP();
    // printf("WR Done!\n");
    // sleep_ms(100);
    // printf("WR2 TX!\n");
    // tusb_control_request_t req2 = {.bmRequestType = {0x80}, .bRequest = 0x06, .wValue = 0x0100, .wIndex = 0x0000, .wLength = 0x0012};
    // sendSetup(0x0C, 0x00, req2, received);
    // printf("WR Done!\n");
    while (true) {
    }
    return 0;
}