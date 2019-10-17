#pragma once
#include "core/types.h"

class Controllers
{
private:
public:
  void StrobeJoyPad(u8 val);
  u8 GetJoypad(u8 index);
};
