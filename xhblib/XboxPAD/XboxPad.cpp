#include "stdafx.h"
#include "XboxPad.h"

bool XPAD_IsButtonPressed(uint8_t userIndex, uint16_t buttonsMask) {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Cast userIndex to DWORD for XInput
    DWORD result = XInputGetState(static_cast<DWORD>(userIndex), &state);

    if (result != ERROR_SUCCESS)
    {
        // Controller not connected
        return false;
    }

    // Cast buttonsMask to WORD when comparing with XInput struct
    return (state.Gamepad.wButtons & static_cast<WORD>(buttonsMask)) == buttonsMask;
}


// if left, set to 1, if right, set to 0
uint8_t XPAD_GetTriggerValue(uint8_t userIndex, bool leftOrRight) {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Cast userIndex to DWORD for XInput
    DWORD result = XInputGetState(static_cast<DWORD>(userIndex), &state);

    if (result != ERROR_SUCCESS)
    {
        // Controller not connected
        return 0;
    }

	if (leftOrRight) {
		return state.Gamepad.bLeftTrigger;
	} else {
		return state.Gamepad.bRightTrigger;
	}
}

int16_t XPAD_GetStickAxisValue(uint8_t userIndex, uint8_t axis) {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Cast userIndex to DWORD for XInput
    DWORD result = XInputGetState(static_cast<DWORD>(userIndex), &state);

    if (result != ERROR_SUCCESS)
    {
        // Controller not connected
        return 0;
    }

	switch (axis) {
        case 1: return state.Gamepad.sThumbLX; // Left stick X axis
        case 2: return state.Gamepad.sThumbLY; // Left stick Y axis
        case 3: return state.Gamepad.sThumbRX; // Right stick X axis
        case 4: return state.Gamepad.sThumbRY; // Right stick Y axis
        default: return 0; // Invalid axis
    }
}

void XPAD_Disconnect(uint8_t userIndex) {
    XInputdPowerDownDevice(0x10000000 | userIndex);
}

void XPAD_Vibrate(uint8_t userIndex, uint16_t leftMotor, uint16_t rightMotor) {
    XINPUT_VIBRATION vibrate;
    ZeroMemory(&vibrate, sizeof(XINPUT_VIBRATION));

    vibrate.wLeftMotorSpeed = leftMotor;
    vibrate.wRightMotorSpeed = rightMotor;

    DWORD result = XInputSetState(static_cast<DWORD>(userIndex), &vibrate);

    if (result != ERROR_SUCCESS) {
        // Optionally log or handle error
    }
}