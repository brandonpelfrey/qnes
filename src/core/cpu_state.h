#pragma once

#include "types.h"

struct CPUState
{
  u8 A, X, Y;
  u16 PC;
  u8 SP;

  union {
    u8 P;
    struct
    {
      u8 Carry : 1;
      u8 Zero : 1;
      u8 Interrupt : 1;
      u8 Decimal : 1;
      u8 SoftwareInterrupt : 1;
      u8 Unused : 1; // Should always be 1
      u8 Overflow : 1;
      u8 Sign : 1;
    } Status;
  };
};