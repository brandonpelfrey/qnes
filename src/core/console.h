#pragma once

#include <string>

#include "./bus.h"
#include "./cpu.h"

class Console
{
private:
  std::unique_ptr<Bus> bus;
  std::unique_ptr<CPU> cpu;

public:
  Console();
  void LoadROM(const std::string file_path);
  void HardReset();
  void SoftReset();

  void Test1();
};