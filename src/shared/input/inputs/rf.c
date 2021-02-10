#include "rf.h"
#include "avr-nrf24l01/src/nrf24l01-mnemonics.h"
#include "avr-nrf24l01/src/nrf24l01.h"
#include "config/eeprom.h"
#include <string.h>

#include "output/controller_structs.h"
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <stdint.h>

bool write_config;
volatile bool rf_interrupt = false;
uint32_t generate_crc32(void) {
  uint32_t crc = 0x01234567;
  int i, j;
  for (i = 0; i < 32; i++) {
    crc = crc ^ boot_signature_byte_get(i);
    for (j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc = crc >> 1;
    }
  }
  return crc;
}
#ifdef __AVR_ATmega32U4__
#  define CE 0
#  define CSN 10
#else
#  define CE 8
#  define CSN PIN_SPI_SS
#endif
void nrf24_ce_digitalWrite(uint8_t state) { digitalWrite(CE, state); }
void nrf24_csn_digitalWrite(uint8_t state) { digitalWrite(CSN, state); }
void initRF(bool tx, uint32_t txid, uint32_t rxid) {
  rf_interrupt = tx;

  /* init hardware pins */
  if (CE != PIN_SPI_SS) { pinMode(CE, OUTPUT); }
  pinMode(CSN, OUTPUT);
  nrf24_init();

  nrf24_config(76, tx);
  nrf24_tx_address((uint8_t *)&txid);
  nrf24_rx_address((uint8_t *)&rxid);
  // interrupt on falling edge of INT
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__) 
    EICRA |= _BV(ISC41);
    EIMSK |= _BV(INT4);
#elif defined(__AVR_ATmega32U4__)
    EICRA |= _BV(ISC31);
    EIMSK |= _BV(INT3);
#else
    EICRA |= _BV(ISC01);
    EIMSK |= _BV(INT0);
#endif
}

int tickRFTX(uint8_t *data, uint8_t *arr, uint8_t len) {
  bool ret = 0;
  rf_interrupt = false;
  uint8_t status = nrf24_getStatus();
  if (((status & 0B1110) >> 1) == 0) {
    ret = 1;
    nrf24_getData(arr, 0);
    nrf24_configRegister(STATUS, (1 << RX_DR));
  }
  nrf24_send(data, len);
  return ret;
}
uint8_t id = 0;
uint8_t tickRFInput(uint8_t *data, uint8_t len) {
  if (rf_interrupt) {
    rf_interrupt = false;
    uint8_t status = nrf24_getStatus();
    if (((status & 0B1110) >> 1) != 0x7) {
      return nrf24_getData(data, len);
    }
  }
  return false;
}
#if defined(__AVR_ATmega32U4__)
ISR(INT3_vect) {
#else
ISR(INT0_vect) {
#endif
  rf_interrupt = true;
}
