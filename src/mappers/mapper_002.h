#pragma once

#include "core/cartridge.h"

class Mapper_002 : public Cartridge
{
private:
  CartridgeDescription description;

  int selected_bank;

public:
  Mapper_002(CartridgeDescription description) noexcept;

  bool CPURead(u16 addr, u8 &val) final;
  bool CPUWrite(u16 addr, u8 val) final;

  bool PPURead(u16 addr, u8 &val) final;
  bool PPUWrite(u16 addr, u8 val) final;
};