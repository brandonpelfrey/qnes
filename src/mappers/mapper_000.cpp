#include "mappers/mapper_000.h"
#include "core/types.h"

// https://wiki.nesdev.com/w/index.php/NROM

Mapper_000::Mapper_000(CartridgeDescription description) noexcept
    : description(description)
{
}

bool Mapper_000::CPURead(u16 addr, u8 &val)
{
  // CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch
  // CPU $8000-$BFFF: First 16 KB of ROM.
  // CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
  if (addr >= 0x8000)
  {
    u16 base_address = 0x8000;
    if (description.PRG_ROM_16KB_Multiple == 1 && addr >= 0xC000)
      base_address = 0xC000;

    u16 offset = addr - base_address;
    val = PRG_ROM[offset];

    return true;
  }
  else
  {
    return false;
  }
}

bool Mapper_000::CPUWrite(u16 addr, u8 val)
{
  return false;
}

bool Mapper_000::PPURead(u16 addr, u8 &val)
{
  if (addr <= 0x2000 && description.CHR_ROM_8KB_Multiple > 0)
  {
    val = CHR_ROM[addr % (description.CHR_ROM_8KB_Multiple * 0x2000)];
    return true;
  }

  return false;
}

bool Mapper_000::PPUWrite(u16 addr, u8 val)
{
  return false;
}
