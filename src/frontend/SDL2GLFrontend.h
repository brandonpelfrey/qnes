#pragma once

#include <thread>
#include "frontend/Frontend.h"
#include "core/types.h"
#include "./imgui_context.h"

struct SDL_Window;
class CPUWindow;
class PPUWindow;

class SDL2GLFrontend : public Frontend
{
private:
  SDL_Window *window;
  bool should_close;
  u32 framebuffer_texture;
  ImGuiContext *imgui_context;
  std::shared_ptr<Console> console;

  CPUWindow *cpu_window;
  PPUWindow *ppu_window;

  bool show_cpu_window;
  bool show_ppu_window;

private:
  void imgui();

public:
  SDL2GLFrontend(std::shared_ptr<Console> console) noexcept;
  ~SDL2GLFrontend() ;

  void MainLoop();
  void AwaitExit();
};