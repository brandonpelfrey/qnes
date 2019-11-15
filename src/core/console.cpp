#include <memory>
#include "console.h"
#include "core/trace_event.h"

Console::Console()
    : bus(std::make_shared<Bus>()),
      cpu(std::make_shared<CPU>()),
      ppu(std::make_shared<PPU>()),
      controllers(std::make_shared<Controllers>())
{
  // Everything get's a pointer to the bus
  cpu->SetBus(bus);
  ppu->SetBus(bus);

  // And the bus gets to point at everything
  bus->SetCPU(cpu);
  bus->SetPPU(ppu);
  bus->SetControllers(controllers);
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

int Console::StepCPU()
{
  TraceEvent cpu_step;
  int cpuCycles = cpu->Step();
  TraceEventEmitter::Instance()->Emit(cpu_step, "CPUStep");

  for (int ppu_cycles = 0; ppu_cycles < 3 * cpuCycles; ++ppu_cycles)
  {
    ppu->Clock();
  }

  return cpuCycles;
}

void Console::StepFrame()
{
  bool should_continue = true;
  ppu->SetEndFrameCallBack([&]() { should_continue = false; frame_count++; });

  while (should_continue)
  {
    cpu_clock_count += StepCPU();
    if (cpu->IsPaused())
      break;
  }
}