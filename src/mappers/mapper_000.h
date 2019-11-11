#pragma once

#include "core/cartridge.h"

class Mapper_000 : public Cartridge
{
private:
  CartridgeDescription description;

public:
  Mapper_000(CartridgeDescription description) noexcept;

  bool CPURead(u16 addr, u8 &val) final;
  bool CPUWrite(u16 addr, u8 val) final;

  bool PPURead(u16 addr, u8 &val) final;
  bool PPUWrite(u16 addr, u8 val) final;
};