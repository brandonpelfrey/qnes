#pragma once

#include <memory>
#include "core/types.h"

struct CartridgeDescription
{
  u8 PRG_ROM_16KB_Multiple;
  u8 CHR_ROM_8KB_Multiple;

  bool HardwiredMirroringMode;
  bool HasBatteryBackedRAM;
  bool IgnoreMirroringControl;
  u8 MapperNumber;
};

class Cartridge
{
protected:
  CartridgeDescription description;
  u8 *PRG_ROM;
  u8 *CHR_ROM;

public:
  static Cartridge *LoadRomFile(const char *path);
  const CartridgeDescription &GetDescription() const { return description; }

  ~Cartridge()
  {
    delete[] PRG_ROM;
    delete[] CHR_ROM;
  }

  virtual bool CPURead(u16 addr, u8 &val) = 0;
  virtual bool CPUWrite(u16 addr, u8 val) = 0;

  virtual bool PPURead(u16 addr, u8 &val) = 0;
  virtual bool PPUWrite(u16 addr, u8 val) = 0;
};