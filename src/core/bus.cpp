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
    address = 0x2000 | (address & 7);
    return ppu->Read(address);
  }
  else if (address >= 0x4000 && address < 0x4013)
  {
    assert(0 && "APU and IO read not implemented");
  }
  else if (address == 0x4016 || address == 0x4017)
  {
    printf("Controllers queried\n");
    // TODO : controllers->GetJoypad();
  }
  else if (address < 0x4020)
  {
    // This is only allowed in "CPU Test Mode"
    // https://wiki.nesdev.com/w/index.php/CPU_Test_Mode
    return 0;
  }
  else
  {
    return cartridge->Read(address);
  }
}

void Bus::Write(u16 address, u8 val)
{
  if (address < 0x2000)
  {
    address &= 0x7FF;
    RAM[address] = val;
  }
  else if (address < 0x4000)
  {
    // PPU Registers 8 bytes mirrored
    address = 0x2000 | (address & 7);
    ppu->Write(address, val);
  }
  else if (address == 0x4014)
  {
    // OAM DMA
    ppu->Write(address, val);
  }
  else if (address == 0x4016 || address == 0x4017)
  {
    controllers->StrobeJoyPad(val);
  }
  else
  {
    printf("Unimplemented bus write @ 0x%04X\n", address);
    //assert(0 && "Write not implemented outside ram");
  }
}

void Bus::TriggerNMI()
{
  cpu->TriggerNMI();
}