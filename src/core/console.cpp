#include <memory>
#include "console.h"

Console::Console()
    : bus(std::make_unique<Bus>()),
      cpu(std::make_unique<CPU>())
{
  cpu->SetBus(bus.get());
}

void Console::Test1()
{
  cpu->Reset();
  bus->Write(0x0000, 0xE8); // INX
  bus->Write(0x0001, 0xE0); // CPX
  bus->Write(0x0002, 0x03); // (val)
  bus->Write(0x0003, 0xD0); // BNE
  bus->Write(0x0004, 0xFB); // (val)
  bus->Write(0x0005, 0xC8); // INY
  bus->Write(0x0006, 0xC8); // INY
  bus->Write(0x0007, 0xC8); // INY

  for (int cyc = 0; cyc < 100; ++cyc)
  {
    cpu->Debug();
    cpu->Step();
  }
}