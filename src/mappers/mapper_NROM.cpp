#include "mappers/mapper_NROM.h"
#include "core/types.h"

// https://wiki.nesdev.com/w/index.php/NROM

u8 Mapper_NROM::Read(u16 addr)
{
  // CPU $6000-$7FFF: Family Basic only: PRG RAM, mirrored as necessary to fill entire 8 KiB window, write protectable with an external switch
  // CPU $8000-$BFFF: First 16 KB of ROM.
  // CPU $C000-$FFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128).
  if (addr >= 0x8000)
  {
    u16 base_address = 0x8000;
    if (description.CHR_ROM_8KB_Multiple == 1 && addr >= 0xC000)
      base_address = 0xC000;

    u16 offset = addr - base_address;
    u8 val = PRG_ROM[offset];
    //printf("NROM read 0x%04X (0x%04X) = 0x%02X\n", addr, offset, val);

    return val;
  }
  else
  {
    printf("Unhandled NROM read @ 0x%04X\n", addr);
    assert(0);
  }
}

void Mapper_NROM::Write(u16 addr, u8 val)
{
  // Doesn't do anything on NROM...
  return;
}