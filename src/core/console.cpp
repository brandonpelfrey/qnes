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
  this->ppu->SetCartridge(this->cartridge);
}

void Console::HardReset()
{
  cpu->Reset();
  frame_count = 0;
  cpu_clock_count = 0;
}

void Console::StepFrame()
{
  int starting_frame_count = frame_count;

  ppu->SetEndFrameCallBack([&]() {
    //printf("frame %u\n", frame_count);
    frame_count++;
  });

  while (frame_count == starting_frame_count)
  {
    int cpuCycles = cpu->Step();
    cpu_clock_count += cpuCycles;

    for (int ppu_cycles = 0; ppu_cycles < 3 * cpuCycles; ++ppu_cycles)
    {
      ppu->Clock();
    }
  }
}

void Console::Test2()
{
  printf("Reset Vector (0xFFFC) : 0x%02X%02X\n", bus->Read(0xFFFD), bus->Read(0xFFFC));
  cpu->Reset();
}