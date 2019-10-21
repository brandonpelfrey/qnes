#pragma once

#include "core/cartridge.h"

class Mapper_NROM : public Cartridge
{
public:
  bool CPURead(u16 addr, u8 &val) final;
  bool CPUWrite(u16 addr, u8 val) final;

  bool PPURead(u16 addr, u8 &val) final;
  bool PPUWrite(u16 addr, u8 val) final;
};