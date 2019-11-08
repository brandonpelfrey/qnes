#pragma once

#include <thread>
#include "frontend/Frontend.h"
#include "core/types.h"
#include "./imgui_context.h"

struct SDL_Window;
class SDL2GLFrontend : public Frontend
{
private:
  SDL_Window *window;
  bool should_close;
  u32 framebuffer_texture;

private:
  void imgui();

public:
  SDL2GLFrontend() noexcept;

  void MainLoop();
  void AwaitExit();
};