#pragma once

#include <string>

class Console
{
private:
public:
  Console();
  void LoadROM(const std::string file_path);
  void HardReset();
  void SoftReset();
};