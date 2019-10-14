#pragma once

#include <memory>
#include "./types.h"
#include "./cpu.h"
#include "./ppu.h"

class Bus
{
private:
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<PPU> ppu;
  u8 RAM[2048];

public:
  u8 Read_U8(u16 address);
  void Write_U8(u16 address, u8 val);
};