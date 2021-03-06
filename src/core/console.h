#pragma once

#include <string>

#include "./bus.h"
#include "./cpu.h"
#include "./ppu.h"
#include "./cartridge.h"
#include "./texture.h"

class Console
{
private:
  std::shared_ptr<Bus> bus;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<Controllers> controllers;

  u64 cpu_clock_count;
  u32 frame_count;

public:
  Console();

  void LoadROM(const char *file_path);
  void HardReset();
  void SoftReset();
  void StepFrame();
  int StepCPU();

  void Test1();
  void Test2();

  std::shared_ptr<CPU> GetCPU() { return cpu; }
  u64 GetCPUClockCount() const { return cpu_clock_count; }
  u32 GetFrameCount() const { return frame_count; }
  Texture& GetFrameBuffer() { return ppu->GetFrameBufferTexture(); }
  std::shared_ptr<PPU> GetPPU() { return ppu; }
  std::shared_ptr<Controllers> GetControllers() { return controllers; }
  std::shared_ptr<Bus> GetBus() { return bus; }
};