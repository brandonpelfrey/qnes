#include <map>
#include <algorithm>
#include <imgui.h>
#include <SDL_opengl.h>

#include "core/cpu_debug.h"
#include "core/state.h"
#include "frontend/window_ppu.h"
#include "frontend/imgui_memory_editor.h"

static MemoryEditor ppu_mem_edit_window;

/*
void HelperText(const char *text)
{
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("%s", text);
}
*/

PPUWindow::PPUWindow(std::shared_ptr<Console> console, ImFont *font)
    : Window(console, font)
{
  ppu_mem_edit_window.Cols = 8;
  ppu_mem_edit_window.GetLastPCForWrite = [&](u16 addr) -> u16 {
    return 0;
  };

  state = new PPURegisterState;

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &gl_tex_left_pattern);
  glBindTexture(GL_TEXTURE_2D, gl_tex_left_pattern);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glGenTextures(1, &gl_tex_right_pattern);
  glBindTexture(GL_TEXTURE_2D, gl_tex_right_pattern);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glGenTextures(1, &gl_tex_nametable);
  glBindTexture(GL_TEXTURE_2D, gl_tex_nametable);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  return;
}

PPUWindow::~PPUWindow()
{
  glDeleteTextures(1, &gl_tex_left_pattern);
  glDeleteTextures(1, &gl_tex_right_pattern);
  glDeleteTextures(1, &gl_tex_nametable);
  delete state;
}

void PPUWindow::render_registers()
{
  const ImVec4 color_active(1.0f, 1.0f, 1.0f, 1.0f);
  const ImVec4 color_inactive(1.0f, 0.80f, 0.80f, 0.4f);

  m_console->GetPPU()->GetState(state);

  /* Status Register / Saved Status Register + Misc. Registers */
  {
    ImGui::Text("Status: ");

    ImGui::SameLine();
    ImGui::Text("    PPUCTRL %02X", state->PPUCTRL);

    ImGui::SameLine();
    ImGui::Text("    PPUMASK %02X", state->PPUMASK);

    ImGui::SameLine();
    ImGui::Text("    PPUSTATUS %02X", state->PPUSTATUS);

    ImGui::SameLine();
    ImGui::Text("    OAMADDR %02X", state->OAMADDR);

    ImGui::SameLine();
    ImGui::Text("    VRAM %04X", state->vram_addr);

    ImGui::SameLine();
    ImGui::Text("    Raster X,Y (%d,%d)", state->pixel_x, state->pixel_y);
  }
}

void PPUWindow::render(bool embed)
{
  ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.95f;
  ImGui::PushFont(m_font);

  ImGui::SetNextWindowSizeConstraints(ImVec2(1175, 300), ImVec2(1175, 1600));
  ImGui::SetNextWindowSize(ImVec2(1175, 600), ImGuiCond_FirstUseEver);
  if (!embed && !ImGui::Begin("PPU Debugger", NULL, ImGuiWindowFlags_NoScrollbar))
  {
    ImGui::End();
    ImGui::PopFont();
    return;
  }

  render_registers();
  ImGui::Separator();
  ImGui::Columns(3);

  const auto draw_texture = [&](Texture &texture, int gl_texture, int dim) {
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.GetWidth(), texture.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, texture.Data());
    float ratio = texture.GetHeight() / texture.GetWidth();
    ImGui::Image((void *)(intptr_t)gl_texture, ImVec2(dim, dim * ratio));
  };

  /* Disassembly view */
  {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float min_dim = std::min(avail.x * 0.9f, avail.y * 0.5f * 0.9f);

    draw_texture(m_console->GetPPU()->GetPatternTableLeftTexture(), gl_tex_left_pattern, min_dim);
    draw_texture(m_console->GetPPU()->GetPatternTableRightTexture(), gl_tex_right_pattern, min_dim);
  }

  ImGui::NextColumn();

  /* Breakpoints and shortcuts */
  {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float min_dim = std::min(avail.x * 0.9f, avail.y * 0.5f * 0.9f);

    draw_texture(m_console->GetPPU()->GetNametablesTexture(), gl_tex_nametable, min_dim);
  }

  ImGui::NextColumn();

  {
    ImGui::BeginChild("ppu_memory_editor", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
    ppu_mem_edit_window.DrawContents(m_console->GetPPU()->GetVRAM(), 0x4000, 0);
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
