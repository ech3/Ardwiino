#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
typedef struct {
    uint8_t pin;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Led_t;
typedef struct {
    int8_t multiplier;
    int8_t offset;
    int8_t deadzone;
} AxisScale_t;
typedef struct {
    uint8_t note : 7;
    bool isCC : 1;
    uint8_t channel : 4;
} Midi_t;
typedef struct {
    AxisScale_t scale;
    bool mapToDigital;
    uint8_t trigger;
    uint8_t channel;
} AnalogData_t;
typedef struct {
    uint8_t binding;
    bool pulldown;
    bool pullup;
    Led_t led;
    Midi_t midi;
    uint8_t analogID; //In the tool itself, we can store a list of what each analog pin maps to, so each analog pin has a unique id in the analog list
} Binding_t;

typedef struct {
    Binding_t bindings[PORTS * PINS_PER_PORT];
    AnalogData_t analog[NUM_ANALOG_INPUTS];
    bool mapLeftJoystickToDPad;
    bool mapStartSelectToHome;
    bool mapNunchukAccelToRightJoy;
} Bindings_t;

typedef struct {
    uint8_t inputType;
    uint8_t subType;
    uint8_t tiltType;
    uint8_t pollRate;
    uint8_t fretLEDMode;
    uint32_t signature;
} MainConfig_t;

typedef struct {
    uint8_t mpu6050Orientation;
    int16_t tiltSensitivity;
} AxisConfig_t;

typedef struct {
    bool rfInEnabled;
    uint32_t id;
} RFConfig_t;


typedef struct {
    MainConfig_t main;
    AxisConfig_t axis;
    RFConfig_t rf;
} Configuration_t;