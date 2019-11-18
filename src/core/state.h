#pragma once

#include "types.h"

struct State
{
  u8 a, x, y, p, s;
  u16 pc;
};

struct PPURegisterState
{
  u16 pixel_y;
  u16 pixel_x;
  u8 nmi_latch;
  u8 address_latch;
  u8 scroll_x;
  u8 scroll_y;
  u16 vram_addr;

  u8 PPUCTRL;
  u8 PPUSTATUS;
  u8 PPUMASK;
  u8 OAMADDR;
};