#pragma once

#include "core/console.h"
#include "frontend/window.h"

struct PPURegisterState;
struct ImFont;

class PPUWindow : public Window
{
public:
  PPUWindow(std::shared_ptr<Console> console, ImFont *font);
  ~PPUWindow();

  void SetConsole(std::shared_ptr<Console> console)
  {
    m_console = console;
  }

private:
  PPURegisterState *state;

  u32 gl_tex_left_pattern;
  u32 gl_tex_right_pattern;
  u32 gl_tex_nametable;

  void render_registers();
  void render_disassembly(int disassembly_lines);

  void render(bool embed) override;

  static const int DISASSEMBLY_ENTRIES = 256;
  static const int DISASSEMBLY_LENGTHS = 512;
  std::vector<CPU::DisassemblyEntry> dis_entries;
};
