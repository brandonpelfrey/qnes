#pragma once

#include "core/console.h"
#include "frontend/window.h"

class ImFont;
class CPUWindow : public Window
{
public:
  CPUWindow(std::shared_ptr<Console> console,
            std::atomic<uint64_t> *m_cpu_continue,
            ImFont *font);

  void SetConsole(std::shared_ptr<Console> console)
  {
    m_console = console;
  }

private:
  std::atomic<uint64_t> *const m_cpu_continue;
  std::vector<u32> m_breakpoints;

  void breakpoint_add(u32 address);
  void breakpoint_remove(u32 address);
  bool breakpoint_check(u32 address) const;

  void render_registers();
  void render_disassembly(int disassembly_lines);

  void render(bool embed) override;
};
