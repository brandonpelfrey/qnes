#pragma once

#include "core/cartridge.h"

class Mapper_001 : public Cartridge
{
private:
  CartridgeDescription description;

  // CPU $6000-$7FFF: 8 KB PRG RAM bank, fixed on all boards but SOROM and SXROM
  u8 *PRG_RAM;
  u8 ControlRegister;

  int PRGSelect = 0;
  int CHR0Select = 0;
  int CHR1Select = 0;

  u16 PRGOffsets[2] = {0, 0};
  u16 CHROffsets[2] = {0, 0};

  // CPU $8000-$FFFF is connected to a common shift register.
  u8 shift_register = 0x00;
  int shift_register_write_counter = 0;

  void updateOffsets();

public:
  Mapper_001(CartridgeDescription description) noexcept;
  ~Mapper_001();

  bool CPURead(u16 addr, u8 &val) final;
  bool CPUWrite(u16 addr, u8 val) final;

  bool PPURead(u16 addr, u8 &val) final;
  bool PPUWrite(u16 addr, u8 val) final;
};