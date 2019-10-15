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
  u8 RAM[0x0800];

public:
  u8 Read(u16 address, bool affects_state=true);
  void Write(u16 address, u8 val);
};