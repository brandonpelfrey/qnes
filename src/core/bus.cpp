#include <cassert>
#include "core/bus.h"

u8 Bus::Read(u16 address, bool affects_state)
{
  if (address < 0x2000)
  {
    // 2KB of memory @ range [0,0x7FF], mirrored 3 more times
    return RAM[address & 0x7FF];
  }
  else if (address < 0x4000)
  {
    // PPU Registers 8 bytes mirrored
    // effective_address = 0x2000 | (address & 7)
    assert(0 && "PPU read not implemented");
  }
  else if (address < 0x4018)
  {
    assert(0 && "APU and IO read not implemented");
  }
  else if (address < 0x4020)
  {
    // This is only allowed in "CPU Test Mode"
    // https://wiki.nesdev.com/w/index.php/CPU_Test_Mode
    return 0;
  }
  else
  {
    assert(0 && "Reads from cartridge not implemented");
  }
}

void Write(u16 address, u8 val)
{
}