#include "stdafx.h"
#include "XboxPad.h"
bool XPAD_IsButtonPressed(uint8_t userIndex, uint16_t buttonsMask)
{
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

void XPAD_Disconnect(uint8_t userIndex) {
    if (userIndex > 3) return; // ignore invalid index
    XInputdPowerDownDevice(0x10000000 | userIndex);
}