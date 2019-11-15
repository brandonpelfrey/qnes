#include "mappers/mapper_003.h"
#include "core/types.h"

Mapper_003::Mapper_003(CartridgeDescription description) noexcept
    : description(description)
{
  selected_bank = 0;
}

// NNROM
bool Mapper_003::CPURead(u16 addr, u8 &val)
{
  if (addr >= 0x8000)
  {
    u16 base_address = 0x8000;
    if (description.PRG_ROM_16KB_Multiple == 1 && addr >= 0xC000)
      base_address = 0xC000;

    u16 offset = addr - base_address;
    val = PRG_ROM[offset];

    return true;
  }

  return false;
}

bool Mapper_003::CPUWrite(u16 addr, u8 val)
{
  if (addr < 0x8000)
    return false;

  selected_bank = val;
  return true;
}

bool Mapper_003::PPURead(u16 addr, u8 &val)
{
  if (addr < 0x2000 && description.CHR_ROM_8KB_Multiple > 0)
  {
    val = CHR_ROM[0x2000 * selected_bank + addr];
    return true;
  }

  return false;
}

bool Mapper_003::PPUWrite(u16 addr, u8 val)
{
  return false;
}
