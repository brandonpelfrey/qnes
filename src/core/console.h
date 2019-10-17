#pragma once

#include <string>

#include "./bus.h"
#include "./cpu.h"
#include "./ppu.h"
#include "./cartridge.h"

class Console
{
private:
  std::shared_ptr<Bus> bus;
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<Cartridge> cartridge;

public:
  Console();

  void LoadROM(const char *file_path);
  void HardReset();
  void SoftReset();

  void Test1();
  void Test2();
};