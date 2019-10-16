#pragma once

#include "core/cartridge.h"

class Mapper_NROM : public Cartridge
{
public:
  u8 Read(u16 addr) final;
  void Write(u16 addr, u8 val) final;
};