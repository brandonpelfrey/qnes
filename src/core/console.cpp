#include <memory>
#include "console.h"

Console::Console()
    : bus(std::make_shared<Bus>()),
      cpu(std::make_shared<CPU>()),
      ppu(std::make_shared<PPU>())
{
  // Everything get's a pointer to the bus
  cpu->SetBus(bus);
  ppu->SetBus(bus);

  // And the bus gets to point at everything
  bus->SetCPU(cpu);
  bus->SetPPU(ppu);
}

void Console::LoadROM(const char *path)
{
  this->cartridge = std::shared_ptr<Cartridge>(Cartridge::LoadRomFile(path));
  this->bus->SetCartridge(this->cartridge);
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

void Console::Test2()
{
  printf("Reset Vector (0xFFFC) : 0x%02X%02X\n", bus->Read(0xFFFD), bus->Read(0xFFFC));

  uint64_t total = 0;

  int frame_counter = 0;
  ppu->SetEndFrameCallBack([&]() {
    printf("frame %u -- cpu @ %llu\n", frame_counter, total);
    frame_counter++;
  });

  cpu->Reset();
  for (int cyc = 0; cyc < 10 * 1000 * 1000 && frame_counter < 60; ++cyc)
  {
    cpu->Debug();
    int cpuCycles = cpu->Step();
    total += cpuCycles;
    for (int ppu_cycles = 0; ppu_cycles < 3 * cpuCycles; ++ppu_cycles)
    {
      ppu->Clock();
    }
  }
}