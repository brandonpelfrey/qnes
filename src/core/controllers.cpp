
#include <cstdio>
#include "core/controllers.h"

void Controllers::StrobeJoyPad(u8 index)
{
  controller_shift_registers[index] = controller_states[index];
}

u8 Controllers::ShiftJoyPadBit(u8 index)
{
  u8 val = controller_shift_registers[index] & 1;
  controller_shift_registers[index] >>= 1;

  return val;
}

void Controllers::SetButtonPressed(u8 controller_index, Controllers::ButtonMask button, bool pressed)
{
  if (pressed)
    controller_states[controller_index] |= (button & 0xFF);
  else
    controller_states[controller_index] &= (button ^ 0xFF);
}