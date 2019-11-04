#pragma once
#include "core/types.h"

class Controllers
{
private:
  u8 controller_states[2];
  u8 controller_shift_registers[2];

public:
  enum ButtonMask
  {
    A = 1 << 0,
    B = 1 << 1,
    Select = 1 << 2,
    Start = 1 << 3,
    Up = 1 << 4,
    Down = 1 << 5,
    Left = 1 << 6,
    Right = 1 << 7
  };

  void StrobeJoyPad(u8 val);
  u8 ShiftJoyPadBit(u8 index);
  void SetButtonPressed(u8 controller_index, ButtonMask button, bool pressed);
};
