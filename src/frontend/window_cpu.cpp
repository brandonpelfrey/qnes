#include <map>
#include <algorithm>
#include <imgui.h>
//#include "imgui_impl.h"
//#include "imgui_fonts.h"

#include "core/cpu_debug.h"
#include "core/state.h"
#include "frontend/window_cpu.h"
#include "frontend/imgui_memory_editor.h"

static MemoryEditor mem_edit_window;

void HelperText(const char *text)
{
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", text);
}

CPUWindow::CPUWindow(std::shared_ptr<Console> console, ImFont *font)
    : Window(console, font)
{
  dis_entries.resize(DISASSEMBLY_ENTRIES);
  for (int i = 0; i < DISASSEMBLY_ENTRIES; ++i)
  {
    dis_entries[i].buffer = new char[DISASSEMBLY_LENGTHS];
    dis_entries[i].buffer_len = DISASSEMBLY_LENGTHS;
  }

  mem_edit_window.Cols = 8;
  mem_edit_window.GetLastPCForWrite = [&](u16 addr) -> u16 {
    if (addr < 0x800)
      return m_console->GetBus()->GetLastRAMWritePC(addr);
    return 0;
  };

  return;
}

void CPUWindow::render_registers()
{
  const ImVec4 color_active(1.0f, 1.0f, 1.0f, 1.0f);
  const ImVec4 color_inactive(1.0f, 0.80f, 0.80f, 0.4f);

  static State state;
  m_console->GetCPU()->GetState(&state);

  /* Status Register / Saved Status Register + Misc. Registers */
  {
    ImGui::Text("Status: ");
    ImGui::SameLine();

#define SHOW_BIT(bitmask, str)                                                  \
  ImGui::TextColored((state.p & bitmask) ? color_active : color_inactive, str); \
  ImGui::SameLine();

    SHOW_BIT(0x80, "N")
    SHOW_BIT(0x40, "V")
    SHOW_BIT(0x20, ".")
    SHOW_BIT(0x10, ".")
    SHOW_BIT(0x08, "D")
    SHOW_BIT(0x04, "I")
    SHOW_BIT(0x02, "Z")
    SHOW_BIT(0x01, "C")
#undef SHOW_BIT

    ImGui::SameLine();
    ImGui::Text("    PC %04X  A %02X  X %02X  Y %02X  SP %02X, P %02X", state.pc, state.a, state.x, state.y, state.s, state.p);

    /*
    u8 nmi_high, nmi_low, reset_high, reset_low;
    m_console->GetBus()->GetCartridge()->CPURead(0xFFFA, nmi_low);
    m_console->GetBus()->GetCartridge()->CPURead(0xFFFB, nmi_high);
    m_console->GetBus()->GetCartridge()->CPURead(0xFFFC, reset_low);
    m_console->GetBus()->GetCartridge()->CPURead(0xFFFD, reset_high);
    ImGui::Text("[NMI @ $%02X%02X, RESET @ $%02X%02X]", nmi_high, nmi_low, reset_high, reset_low);
    */
  }
}

void CPUWindow::render_disassembly(int disassembly_lines)
{
  ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());

  State state;
  m_console->GetCPU()->GetState(&state);
  //printf("0x%04X\n", state.pc);
  m_console->GetCPU()->Disassemble(state.pc - disassembly_lines / 3, disassembly_lines, &dis_entries[0]);

  for (int i = 0; i < disassembly_lines; ++i)
  {
    const auto &entry(dis_entries[i]);
    const bool is_breakpoint = m_console->GetCPU()->DebuggingControls().Has(entry.pc, Breakpoint::EXECUTE);
    const bool is_current = (entry.pc == state.pc);

    if (is_current)
    {
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.2f, 0.9f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else if (is_breakpoint)
    {
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.2f, 0.1f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.25f, 1.0f));
    }
    else
    {
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.2f, 0.1f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    static char line[512];
    char *line_cur = line;
    line_cur += sprintf(line_cur, "%04X  ", entry.pc);

    for (int j = 0; j < 3; ++j)
      if (j < entry.num_instruction_bytes)
        line_cur += sprintf(line_cur, "%02X ", entry.instruction_bytes[j]);
      else
        line_cur += sprintf(line_cur, "   ");

    line_cur += sprintf(line_cur, " %-11s", entry.buffer);

    if (entry.computed_operand != 0xFFFF)
      line_cur += sprintf(line_cur, " # %04X", entry.computed_operand);

    if (ImGui::Selectable(line, is_current))
    {
      if (ImGui::GetIO().KeyShift)
      {
        m_console->GetCPU()->SetPC(entry.pc);
      }
      else
      {
        if (is_breakpoint)
        {
          m_console->GetCPU()->DebuggingControls().Remove(entry.pc);
        }
        else
        {
          m_console->GetCPU()->DebuggingControls().Add(Breakpoint{entry.pc});
        }
      }
    }

    ImGui::PopStyleColor(2);
  }

  ImGui::PopItemWidth();
}

void CPUWindow::render(bool embed)
{
  State state;
  m_console->GetCPU()->GetState(&state);

  ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.95f;
  ImGui::PushFont(m_font);

  ImGui::SetNextWindowSizeConstraints(ImVec2(1175, 300), ImVec2(1175, 1600));
  ImGui::SetNextWindowSize(ImVec2(1175, 600), ImGuiCond_FirstUseEver);
  if (!embed && !ImGui::Begin("QNES Debugger", NULL, ImGuiWindowFlags_NoScrollbar))
  {
    ImGui::End();
    ImGui::PopFont();
    return;
  }

  render_registers();
  ImGui::Separator();
  ImGui::Columns(3);

  /* Disassembly view */
  {
    static char address_input[32] = {};

    // Dissasembly text area
    ImGui::BeginChild("scrolling2", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
    const unsigned disassembly_lines =
        std::min((unsigned)(ImGui::GetContentRegionAvail().y / ImGui::GetTextLineHeightWithSpacing()),
                 (unsigned)DISASSEMBLY_ENTRIES);
    render_disassembly(disassembly_lines);
    ImGui::EndChild();

    // Controls beneath
    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 5);
    ImGui::InputText("##addrinput", address_input, sizeof(address_input));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Goto"))
    {
      const char *input = address_input;
      u32 address;

      if (strlen(input) > 2 && input[0] == '0' && input[1] == 'x')
      {
        input += 2;
      }

      if (sscanf(input, "%x", &address) == 1)
      {
        m_console->GetCPU()->SetPC(address);
      }
      address_input[0] = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Breakpoint"))
    {
      const char *input = address_input;
      u16 address;

      if (strlen(input) > 2 && input[0] == '0' && input[1] == 'x')
      {
        input += 2;
      }

      if (sscanf(input, "%hx", &address) == 1)
      {
        m_console->GetCPU()->DebuggingControls().Add(Breakpoint(address));
        address_input[0] = 0;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Continue"))
    {
      // HACK : Without doing a step, we would immediately pause if we're on a breakpoint right now.
      if (m_console->GetCPU()->DebuggingControls().Has(state.pc))
        m_console->GetCPU()->Step();
      m_console->GetCPU()->Continue();
    }
    HelperText("Exit step mode and continue execution");

    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
      m_console->GetCPU()->Pause();
      m_console->GetCPU()->Step();
    }
    HelperText("Step a single instruction and remain paused");
  }

  ImGui::NextColumn();

  /* Breakpoints and shortcuts */
  {
    static const ImVec4 color_active(0.9f, 0.9f, 0.9f, 1.0f);
    static const ImVec4 color_hit(0.7f, 1.0f, 0.7f, 1.0f);

    ImGui::BeginChild("breakpoint_listing", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

    const auto &breakpoints = m_console->GetCPU()->DebuggingControls().GetAll();
    std::vector<u16> removals;
    for (const auto breakpoint : breakpoints)
    {
      char label[32];
      snprintf(label, sizeof(label), "Remove##%d", breakpoint.first);
      if (ImGui::Button(label))
      {
        removals.push_back(breakpoint.first);
      }

      ImGui::SameLine();
      ImGui::TextColored((breakpoint.first == state.pc) ? color_hit : color_active, "0x%04x",
                         breakpoint.first);
    }

    for (const auto removal : removals)
      m_console->GetCPU()->DebuggingControls().Remove(removal);

    ImGui::EndChild();
    if (ImGui::Button("Reboot"))
    {
      m_console->HardReset();
    }
  }

  ImGui::NextColumn();

  {
    ImGui::BeginChild("memory_editor", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
    mem_edit_window.DrawContents(m_console->GetBus()->GetRAMView(), 0x0800, 0);
    ImGui::EndChild();
  }

  ImGui::NextColumn();

  ImGui::Columns(1);

  if (!embed)
  {
    ImGui::End();
  }
  ImGui::PopFont();
}
