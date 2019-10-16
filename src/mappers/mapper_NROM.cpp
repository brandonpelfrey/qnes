#include "mappers/mapper_NROM.h"

// https://wiki.nesdev.com/w/index.php/NROM

u8 Mapper_NROM::Read(u16 addr)
{
  // CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch
  // CPU $8000-$BFFF: First 16 KB of ROM.
  // CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
  if (addr >= 0x8000)
  {
    if (description.CHR_ROM_8KB_Multiple == 1)
      addr &= 0xBFFF;
    return PRG_ROM.get()[addr - 0x8000];
  }
  else
  {
    assert(0);
  }
}

void Mapper_NROM::Write(u16 addr, u8 val)
{
  // Doesn't do anything on NROM...
  return;
}