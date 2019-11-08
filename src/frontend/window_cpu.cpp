#include <imgui.h>
//#include "imgui_impl.h"
//#include "imgui_fonts.h"

//#include "cpu/debug.h"
#include "core/state.h"
#include "frontend/window_cpu.h"

const int DISASSEMBLY_ENTRIES = 256;
const int DISASSEMBLY_LENGTHS = 512;
static CPU::DisassemblyEntry dis_entries[DISASSEMBLY_ENTRIES];

CPUWindow::CPUWindow(std::shared_ptr<Console> console,
                     std::atomic<uint64_t> *cpu_continue,
                     ImFont *font)
    : Window(console, font), m_cpu_continue(cpu_continue)
{
  for (int i = 0; i < DISASSEMBLY_ENTRIES; ++i)
  {
    dis_entries[i].buffer = new char[DISASSEMBLY_LENGTHS];
    dis_entries[i].buffer_len = DISASSEMBLY_LENGTHS;
  }

  return;
}

/*
void CPUWindow::breakpoint_add(u32 address)
{
  if (m_breakpoints.empty())
  {
    //m_console->cpu()->debug_enable(true);
    //m_console->GetCPU()->DebugEnable
  }

  m_console->cpu()->debug_breakpoint_add(address);
}

void CPUWindow::breakpoint_remove(u32 address)
{
  if (m_breakpoints.size() == 1)
  {
    m_console->cpu()->debug_enable(false);
  }

  m_console->cpu()->debug_breakpoint_remove(address);
}
*/

void CPUWindow::render_registers()
{
  const ImVec4 color_active(1.0f, 1.0f, 1.0f, 1.0f);
  const ImVec4 color_inactive(1.0f, 0.80f, 0.80f, 0.4f);

  static State state;
  m_console->GetCPU()->GetState(&state);

  /* Status Register / Saved Status Register + Misc. Registers */
  {
    auto colorize = [&color_active, &color_inactive](bool active) {
      return active ? color_active : color_inactive;
    };

    ImGui::Text("SR:  ");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x80), "N");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x40), "V");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x20), ".");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x10), ".");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x08), "D");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x04), "I");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x02), "Z");
    ImGui::SameLine();
    ImGui::TextColored(colorize(state.p & 0x01), "C");

    ImGui::SameLine();
    ImGui::Text("    PC %04X  A %02X  X %02X  Y %02X  P %02X", state.pc, state.a, state.x, state.y, state.p);
  }
}

void CPUWindow::render_disassembly(int disassembly_lines)
{
  ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());

  State state;
  m_console->GetCPU()->GetState(&state);
  m_console->GetCPU()->Disassemble(state.pc - disassembly_lines / 3, disassembly_lines, dis_entries);

  for (int i = 0; i < disassembly_lines; ++i)
  {
    const auto &entry(dis_entries[i]);
    const bool is_breakpoint = false; //breakpoint_check(pc);
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
    line_cur += sprintf(line_cur, "%s", entry.buffer);

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
          //breakpoint_remove(pc);
        }
        else
        {
          //breakpoint_add(pc);
        }
      }
    }

    ImGui::PopStyleColor(2);
  }

  ImGui::PopItemWidth();
}

void CPUWindow::render(bool embed)
{
  /* Update list of active CPU breakpoints */
  //m_breakpoints.clear();
  //m_console->cpu()->debug_breakpoint_list(&m_breakpoints);

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

  /* Register States */
  {
    render_registers();
  }

  ImGui::Separator();

  ImGui::Columns(2);

  /* Disassembly view */
  {
    static char address_input[32] = {};

    ImGui::BeginChild("scrolling2", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
    const unsigned disassembly_lines =
        ImGui::GetContentRegionAvail().y / ImGui::GetTextLineHeightWithSpacing();
    render_disassembly(disassembly_lines);
    ImGui::EndChild();

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
      u32 address;

      if (strlen(input) > 2 && input[0] == '0' && input[1] == 'x')
      {
        input += 2;
      }

      if (sscanf(input, "%x", &address) == 1)
      {
        //breakpoint_add(address);
        address_input[0] = 0;
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Halt"))
    {
      m_cpu_continue->store(0);
    }

    ImGui::SameLine();
    if (ImGui::Button("Continue"))
    {
      //m_cpu_continue->store(UINT64_MAX);
      m_console->GetCPU()->Continue();
    }

    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
      //m_cpu_continue->store(1);
      m_console->GetCPU()->Pause();
      m_console->GetCPU()->Step();
    }
  }

  ImGui::NextColumn();

  /* Breakpoints and shortcuts */
  {
    static const ImVec4 color_active(0.9f, 0.9f, 0.9f, 1.0f);
    static const ImVec4 color_hit(0.7f, 1.0f, 0.7f, 1.0f);

    ImGui::BeginChild("scrolling", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

    /*
    const u32 pc = m_console->cpu()->registers().PC;
    for (unsigned i = 0; i < m_breakpoints.size(); ++i)
    {
      char label[32];

      snprintf(label, sizeof(label), "Remove##%d", i);
      if (ImGui::Button(label))
      {
        breakpoint_remove(m_breakpoints[i]);
      }
      ImGui::SameLine();
      ImGui::TextColored((m_breakpoints[i] == pc) ? color_hit : color_active, "0x%08x",
                         m_breakpoints[i]);
    }
    */
    ImGui::EndChild();

    if (ImGui::Button("Reboot"))
    {
      m_console->HardReset();
    }
  }

  ImGui::NextColumn();

  ImGui::Columns(1);

  if (!embed)
  {
    ImGui::End();
  }
  ImGui::PopFont();
}
