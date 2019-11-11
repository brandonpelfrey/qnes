#include "mappers/Mapper_002.h"
#include "core/types.h"

Mapper_002::Mapper_002(CartridgeDescription description) noexcept
    : description(description)
{
  selected_bank = 0;
}

// UNROM
bool Mapper_002::CPURead(u16 addr, u8 &val)
{
  // CPU $8000-$BFFF: 16 KB switchable PRG ROM bank
  // CPU $C000-$FFFF: 16 KB PRG ROM bank, fixed to the last bank

  if (addr < 0x8000)
    return false;

  if (addr >= 0xC000)
    val = PRG_ROM[0x4000 * (description.PRG_ROM_16KB_Multiple - 1) + addr - 0xC000];
  else
    val = PRG_ROM[0x4000 * (selected_bank) + addr - 0x8000];

  return true;
}

bool Mapper_002::CPUWrite(u16 addr, u8 val)
{
  if (addr < 0x8000)
    return false;

  selected_bank = val;
  return true;
}

bool Mapper_002::PPURead(u16 addr, u8 &val)
{
  if (addr <= 0x2000 && description.CHR_ROM_8KB_Multiple > 0)
  {
    val = CHR_ROM[addr % (description.CHR_ROM_8KB_Multiple * 0x2000)];
    return true;
  }

  return false;
}

bool Mapper_002::PPUWrite(u16 addr, u8 val)
{
  return false;
}
