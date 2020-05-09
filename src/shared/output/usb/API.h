#pragma once
#define COMMAND_START_CONFIG 's'
#define COMMAND_APPLY_CONFIG 'a'
#define COMMAND_WRITE_CONFIG_VALUE 'w'
#define COMMAND_READ_CONFIG_VALUE 'r'
#define COMMAND_READ_INFO 'i'
#define COMMAND_JUMP_BOOTLOADER_UNO 'j'
#define COMMAND_JUMP_BOOTLOADER 'e'
#define COMMAND_STK_500_ENTER_PROG '2'
#define COMMAND_SET_GH_LEDS 'l'
#define COMMAND_SET_LED_COLOUR 'g'
#define COMMAND_FIND_DIGITAL 'd'
#define COMMAND_FIND_ANALOG 'n'

#define INFO_VERSION 0
#define INFO_SIGNATURE 1
#define INFO_MAIN_MCU 2
#define INFO_USB_MCU 3
#define INFO_CPU_FREQ 4
#define INFO_USB_CPU_FREQ 5
#define INFO_BOARD 6
#define INFO_EXT 7

#define CONFIG_INPUT_TYPE 0
#define CONFIG_SUB_TYPE 1
#define CONFIG_TILT_TYPE 2
#define CONFIG_MPU_6050_ORIENTATION 3
#define CONFIG_TILT_SENSITIVITY 4
#define CONFIG_POLL_RATE 5
#define CONFIG_LED_TYPE 6
#define CONFIG_CURRENT_SUB_TYPE 7
#define CONFIG_CURRENT_INPUT_TYPE 8

#define CONFIG_MAP_JOY_DPAD 10 
#define CONFIG_MAP_START_SEL_HOME 11
#define CONFIG_MAP_ACCEL_RIGHT 12

#define CONFIG_PIN_UP 20
#define CONFIG_PIN_DOWN 21
#define CONFIG_PIN_LEFT 22
#define CONFIG_PIN_RIGHT 23
#define CONFIG_PIN_START 24
#define CONFIG_PIN_SELECT 25
#define CONFIG_PIN_LEFT_STICK 26
#define CONFIG_PIN_RIGHT_STICK 27
#define CONFIG_PIN_LB 28
#define CONFIG_PIN_RB 29
#define CONFIG_PIN_HOME 30
#define CONFIG_PIN_CAPTURE 31
#define CONFIG_PIN_A 32
#define CONFIG_PIN_B 33
#define CONFIG_PIN_X 34
#define CONFIG_PIN_Y 35
#define CONFIG_PIN_LT 36
#define CONFIG_PIN_RT 37
#define CONFIG_PIN_L_X 38
#define CONFIG_PIN_L_Y 39
#define CONFIG_PIN_R_X 40
#define CONFIG_PIN_R_Y 41

#define CONFIG_KEY_UP 50
#define CONFIG_KEY_DOWN 51
#define CONFIG_KEY_LEFT 52
#define CONFIG_KEY_RIGHT 53
#define CONFIG_KEY_START 54
#define CONFIG_KEY_SELECT 55
#define CONFIG_KEY_LEFT_STICK 56
#define CONFIG_KEY_RIGHT_STICK 57
#define CONFIG_KEY_LB 58
#define CONFIG_KEY_RB 59
#define CONFIG_KEY_HOME 60
#define CONFIG_KEY_CAPTURE 61
#define CONFIG_KEY_A 62
#define CONFIG_KEY_B 63
#define CONFIG_KEY_X 64
#define CONFIG_KEY_Y 65
#define CONFIG_KEY_LT 66
#define CONFIG_KEY_RT 67
#define CONFIG_KEY_L_X 68
#define CONFIG_KEY_L_Y 69
#define CONFIG_KEY_R_X 70
#define CONFIG_KEY_R_Y 71

#define CONFIG_AXIS_INVERT_LT 80
#define CONFIG_AXIS_INVERT_RT 81
#define CONFIG_AXIS_INVERT_L_X 82
#define CONFIG_AXIS_INVERT_L_Y 83
#define CONFIG_AXIS_INVERT_R_X 84
#define CONFIG_AXIS_INVERT_R_Y 85

#define CONFIG_THRESHOLD_JOY 90
#define CONFIG_THRESHOLD_TRIGGER 91
#define CONFIG_THRESHOLD_DRUM 92

#define CONFIG_LED_PINS 101
#define CONFIG_LED_COLOURS 102
#define CONFIG_LED_GH_COLOURS 103