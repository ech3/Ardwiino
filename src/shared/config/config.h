#pragma once
#include <stdbool.h>
#include <stdint.h>
#define ATTR_PACKED __attribute__ ((packed))
typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t back;
  uint8_t left_stick;
  uint8_t right_stick;
  uint8_t LB;
  uint8_t RB;
  uint8_t home;
  uint8_t unused;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t lt;
  uint8_t rt;
  uint8_t l_x;
  uint8_t l_y;
  uint8_t r_x;
  uint8_t r_y;
} ATTR_PACKED pins_t;
typedef struct {
  bool lt;
  bool rt;
  bool l_x;
  bool l_y;
  bool r_x;
  bool r_y;
} ATTR_PACKED axis_invert_t;
typedef struct {
  uint8_t neg;
  uint8_t pos;
} ATTR_PACKED analogue_keys_t;
typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t back;
  uint8_t left_stick;
  uint8_t right_stick;
  uint8_t LB;
  uint8_t RB;
  uint8_t home;
  uint8_t capture;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t lt;
  uint8_t rt;
  analogue_keys_t l_x;
  analogue_keys_t l_y;
  analogue_keys_t r_x;
  analogue_keys_t r_y;
} ATTR_PACKED keys_t;

typedef struct {
  uint8_t version;
  uint8_t input_type;
  uint8_t sub_type;
  uint8_t tilt_type;
  uint8_t pollrate;
  pins_t pins;
  uint16_t mpu_6050_orientation;
  bool frets_led_mode;
  bool map_joy_to_dpad;
  bool map_start_select_to_home;
  bool map_accel_to_right;
  axis_invert_t inversions;
  uint16_t threshold_trigger;
  uint16_t threshold_joy;
  keys_t keys;
  uint32_t cpu_freq;
  uint32_t signature;
} ATTR_PACKED config_t;