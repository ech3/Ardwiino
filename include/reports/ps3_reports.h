#pragma once
#include <stdint.h>

#define PS3_STICK_CENTER 0x80
#define PS3_ACCEL_CENTER 0x0200
#define PS3_REPORT_BUFFER_SIZE  48

typedef struct {
    uint8_t padding;
    uint8_t right_duration;   /* Right motor duration (0xff means forever) */
    uint8_t right_motor_on;   /* Right (small) motor on/off, only supports values of 0 or 1 (off/on) */
    uint8_t left_duration;    /* Left motor duration (0xff means forever) */
    uint8_t left_motor_force; /* left (large) motor, supports force values from 0 to 255 */
} __attribute__((packed)) ps3_rumble_t;

typedef struct {
    uint8_t time_enabled; /* the total time the led is active (0xff means forever) */
    uint8_t duty_length;  /* how long a cycle is in deciseconds (0 means "really fast") */
    uint8_t enabled;
    uint8_t duty_off; /* % of duty_length the led is off (0xff means 100%) */
    uint8_t duty_on;  /* % of duty_length the led is on (0xff mean 100%) */
} __attribute__((packed)) ps3_led_t;

typedef struct {
    uint8_t report_id;
    ps3_rumble_t rumble;
    uint8_t padding[4];
    uint8_t leds_bitmap; /* bitmap of enabled LEDs: LED_1 = 0x02, LED_2 = 0x04, ... */
    ps3_led_t led[4];    /* LEDx at (4 - x) */
    ps3_led_t _reserved; /* LED5, not actually soldered */
} __attribute__((packed)) ps3_output_report;

typedef struct {
    uint8_t outputType;
    uint8_t unknown1;
    uint8_t enable; // 1 to enable, 0 to disable
    uint8_t padding[5];
} ps3_turntable_output_report_t;

typedef struct {
    bool x : 1;  // square
    bool a : 1;  // cross
    bool b : 1;  // circle
    bool y : 1;  // triangle

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpad : 4;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    // Pressure axes for buttons
    // Neutral state is 0x00, max is 0xFF
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadDown;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;

    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3Dpad_Data_t;

typedef struct {
    // Button bits
    bool x : 1;
    bool b : 1;
    bool a : 1;
    bool y : 1;

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t unused2[12];
    uint16_t unused3[4];

} __attribute__((packed)) SwitchGamepad_Data_t;

typedef struct {
    uint8_t reportId;
    uint8_t : 8;
    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool x : 1;  // square, white1
    bool a : 1;  // cross, black1
    bool b : 1;  // circle, black2
    bool y : 1;  // triangle, black3

    bool guide : 1;
    uint8_t : 7;
    uint8_t : 8;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t padding_3[4];
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadDown;
    uint8_t pressureDpadLeft;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;
    uint8_t padding_4[15];
    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)
} __attribute__((packed)) PS3Gamepad_Data_t;
typedef struct {
    // Button bits
    bool x : 1;  // square
    bool a : 1;  // cross
    bool b : 1;  // circle
    bool y : 1;  // triangle

    bool leftShoulder : 1;   // l1
    bool rightShoulder : 1;  // r1
    bool l2 : 1;             // l2
    bool r2 : 1;             // r2

    bool back : 1;  // select
    bool start : 1;
    bool leftThumbClick : 1;   // l3
    bool rightThumbClick : 1;  // r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    // Pressure axes for buttons
    // Neutral state is 0x00, max is 0xFF
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadDown;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;

    // Each of the following are 10 bits in accuracy
    // Centered/neutral state is nominally 0x0200, actual values may vary
    uint16_t accelX;  // Left/right acceleration (roll)
    uint16_t accelZ;  // Forward/back acceleration (pitch)
    uint16_t accelY;  // Up/down acceleration (gravity)
    uint16_t gyro;    // Left/right instantaneous rotation (yaw)

} __attribute__((packed)) PCGamepad_Data_t;

typedef struct
{
    bool x : 1;  // square, blue
    bool a : 1;  // cross, green
    bool b : 1;  // cirlce, red
    bool y : 1;  // triangle, yellow

    bool leftShoulder : 1;   // kick1, l1
    bool rightShoulder : 1;  // kick2, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool padFlag : 1;     // left stick click, l3
    bool cymbalFlag : 1;  // right stick click, r3

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;
    uint8_t blueCymbalVelocity;
    uint8_t yellowCymbalVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t : 8;

    uint16_t unused3[4];
} __attribute__((packed)) PS3RockBandDrums_Data_t;

typedef struct
{
    bool x : 1;  // square, blue
    bool a : 1;  // cross, green
    bool b : 1;  // circle, red
    bool y : 1;  // triangle, yellow

    bool leftShoulder : 1;   // kick, l1
    bool rightShoulder : 1;  // orange, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[8];

    uint8_t yellowVelocity;
    uint8_t redVelocity;
    uint8_t greenVelocity;
    uint8_t blueVelocity;
    uint8_t kickVelocity;
    uint8_t orangeVelocity;

    uint8_t unused2[2];
    uint16_t unused3[4];
} __attribute__((packed)) PS3GuitarHeroDrums_Data_t;

typedef struct
{
    bool y : 1;  // triangle, yellow
    bool a : 1;  // cross, green
    bool b : 1;  // circle, red
    bool x : 1;  // square, blue

    bool leftShoulder : 1;   // orange, l1
    bool rightShoulder : 1;  // spPedal, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t tilt_pc;
    uint8_t unused1;
    uint8_t whammy;
    uint8_t slider;

    uint8_t pressure_dpadRight_yellow;  // yellow
    uint8_t pressure_dpadLeft;
    uint8_t pressure_dpadUp_green;     // green
    uint8_t pressure_dpadDown_orange;  // orange
    uint8_t pressure_blue;
    uint8_t pressure_red;
    uint8_t unused2[6];

    // Reminder that these values are 10-bit in range
    uint16_t tilt;  // accelX
    uint16_t accelZ;
    uint16_t accelY;
    uint16_t unused3;
} __attribute__((packed)) PS3GuitarHeroGuitar_Data_t;

typedef struct
{
    bool x : 1;  // square, blue
    bool a : 1;  // cross, green
    bool b : 1;  // circle, red
    bool y : 1;  // triangle, yellow

    bool leftShoulder : 1;  // orange, l1
    bool tilt : 1;          // tilt, r1
    bool solo : 1;          // l2
    bool : 1;               // r2

    bool back : 1;  // select
    bool start : 1;
    bool : 1;
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t tilt_pc;
    uint8_t unused1;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;
    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;

    uint8_t unused2[10];
    uint16_t unused3[4];
} __attribute__((packed)) PS3RockBandGuitar_Data_t;

typedef struct
{
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t y : 1;  // triangle, euphoria

    uint8_t : 1;
    uint8_t : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t : 1;
    uint8_t : 1;

    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;

    uint8_t unused1[2];
    uint8_t leftTableVelocity;
    uint8_t rightTableVelocity;

    uint8_t pressure_dpadRight_square;
    uint8_t pressure_dpadLeft;
    uint8_t pressure_dpadUp_cross;
    uint8_t pressure_dpadDown;
    uint8_t pressure_triangle;
    uint8_t pressure_circle;
    uint8_t unused2[6];

    // Reminder that these values are 10-bit in range
    uint16_t effectsKnob;
    uint16_t crossfader;
    uint16_t : 6;
    uint16_t tableNeutral : 1;
    uint16_t : 3;
    uint16_t leftBlue : 1;
    uint16_t leftRed : 1;
    uint16_t leftGreen : 1;
    uint16_t rightBlue : 1;
    uint16_t rightRed : 1;
    uint16_t rightGreen : 1;
    uint16_t unused3;
} __attribute__((packed)) PS3Turntable_Data_t;

typedef struct
{
    bool white1 : 1;  // square, white1
    bool black1 : 1;  // cross, black1
    bool black2 : 1;  // circle, black2
    bool black3 : 1;  // triangle, black3

    bool white2 : 1;  // white2, l1
    bool white3 : 1;  // white3, r1
    bool : 1;
    bool : 1;

    bool back : 1;  // back, heroPower
    bool start : 1;      // start, pause
    bool leftThumbClick : 1;       // leftThumbClick, ghtv
    bool : 1;

    bool guide : 1;    // ps
    bool capture : 1;  // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;
    uint8_t : 4;
    uint8_t tilt_pc;   // leftStickX
    uint8_t strumBar;  // leftStickY
    uint8_t unused2;   // rightStickX
    uint8_t whammy;    // rightStickY

    uint8_t unused3[12];

    // Reminder that this value is 10-bit in range
    uint16_t tilt;

    uint16_t unused4[3];
} __attribute__((packed)) PS3GHLGuitar_Data_t;

