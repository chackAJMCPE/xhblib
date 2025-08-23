#pragma once

#include <xbox.h>
#include <cstdint>

extern "C" VOID XInputdPowerDownDevice(DWORD flag);

// Controller indices
#define PAD_1 0
#define PAD_2 1
#define PAD_3 2
#define PAD_4 3

// Button masks
#define BUTTON_UP          0x0001
#define BUTTON_DOWN        0x0002
#define BUTTON_LEFT        0x0004
#define BUTTON_RIGHT       0x0008
#define BUTTON_START       0x0010
#define BUTTON_BACK        0x0020
#define BUTTON_LS          0x0040
#define BUTTON_RS          0x0080
#define BUTTON_LB          0x0100
#define BUTTON_RB          0x0200
// #define BUTTON_BIGBUTTON 0x0800
#define BUTTON_A           0x1000
#define BUTTON_B           0x2000
#define BUTTON_X           0x4000
#define BUTTON_Y           0x8000

#define MOTOR_MAX 65535
#define MOTOR_MIN 0

#define TRIGGER_LEFT 1
#define TRIGGER_RIGHT 0

#define LEFT_X   1  // Left stick X axis
#define LEFT_Y   2  // Left stick Y axis
#define RIGHT_X  3  // Right stick X axis
#define RIGHT_Y  4  // Right stick Y axis

#ifdef __cplusplus
extern "C" {
#endif


bool XPAD_IsButtonPressed(uint8_t userIndex, uint16_t buttonsMask);
uint8_t XPAD_GetTriggerValue(uint8_t userIndex, bool leftOrRight);
int16_t XPAD_GetStickAxisValue(uint8_t userIndex, uint8_t axis);
void XPAD_Disconnect(uint8_t userIndex);
void XPAD_Vibrate(uint8_t userIndex, uint16_t leftMotor, uint16_t rightMotor);

#ifdef __cplusplus
}
#endif
