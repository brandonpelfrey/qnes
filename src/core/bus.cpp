#include <cassert>
#include "core/bus.h"
#include <cstring>

Bus::Bus()
{
  RAM = new u8[0x0800];
  memset(RAM, 0, 0x0800);
  RAMWriteLastPC = new u16[0x0800];
  memset(RAMWriteLastPC, 0, 0x0800);
}

Bus::~Bus()
{
  delete[] RAM;
  delete[] RAMWriteLastPC;
}

u8 Bus::Read(u16 address, bool affects_state)
{
  u8 val;
  if (cartridge->CPURead(address, val))
  {
    return val;
  }
  else if (address < 0x2000)
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
    // TODO assert(0 && "APU and IO read not implemented");
    return 0;
  }
  else if (address == 0x4016 || address == 0x4017)
  {
    return controllers->ShiftJoyPadBit(address & 1);
  }
  else if (address < 0x4020)
  {
    // This is only allowed in "CPU Test Mode"
    // https://wiki.nesdev.com/w/index.php/CPU_Test_Mode
    return 0;
  }
  else
  {
    assert(0);
  }
}

void Bus::Write(u16 address, u8 val)
{
  if (cartridge->CPUWrite(address, val))
  {
    return;
  }
  else if (address < 0x2000)
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
  else if(address <= 0x4015)
  {
    //
  }
  else if (address == 0x4016 || address == 0x4017)
  {
    // TODO : this should actually look at the edge, but for now...
    controllers->StrobeJoyPad(address & 1);
  }
  else
  {
    printf("Unimplemented bus write @ 0x%04X\n", address);
   // assert(0 && "Write not implemented outside ram");
  }
}

void Bus::TriggerNMI()
{
  cpu->TriggerNMI();
}
