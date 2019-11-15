#pragma once

struct SDL_Window;
struct ImGuiContext
{
  SDL_Window * window;
  
public:
  ImGuiContext();
  ~ImGuiContext();

  void StartImGuiFrame();
  void EndImGuiFrame();
};
