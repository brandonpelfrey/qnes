#pragma once

#include "core/console.h"
#include "frontend/window.h"

class ImFont;
class CPUWindow : public Window
{
public:
  CPUWindow(std::shared_ptr<Console> console, ImFont *font);

  void SetConsole(std::shared_ptr<Console> console)
  {
    m_console = console;
  }

private:
  void render_registers();
  void render_disassembly(int disassembly_lines);

  void render(bool embed) override;

  static const int DISASSEMBLY_ENTRIES = 256;
  static const int DISASSEMBLY_LENGTHS = 512;
  std::vector<CPU::DisassemblyEntry> dis_entries;
};
