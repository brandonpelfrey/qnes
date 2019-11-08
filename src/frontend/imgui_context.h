#pragma once

struct SDL_Window;
class ImGuiContext
{
  SDL_Window * window;
  
public:
  ImGuiContext();
  ~ImGuiContext();

  void StartImGuiFrame();
  void EndImGuiFrame();
};
