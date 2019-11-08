#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <functional>

#include <SDL2/SDL.h>
#include "./imgui_context.h"

ImGuiContext::ImGuiContext()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  this->window = SDL_GL_GetCurrentWindow();
  SDL_GLContext gl_context = SDL_GL_GetCurrentContext();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

#ifdef __APPLE__
  ImGui_ImplOpenGL3_Init("#version 150 core");
#else
  ImGui_ImplOpenGL3_Init("#version 150");
#endif

}

ImGuiContext::~ImGuiContext()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiContext::StartImGuiFrame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();
}

void ImGuiContext::EndImGuiFrame()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
