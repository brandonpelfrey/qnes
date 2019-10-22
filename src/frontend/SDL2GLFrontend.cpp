#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <thread>

#include "./SDL2GLFrontend.h"
#include "core/console.h"

SDL2GLFrontend::SDL2GLFrontend() noexcept
{
  should_close = false;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    printf("Error: %s\n", SDL_GetError());
    exit(-1);
  }

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &framebuffer_texture);
}

void SDL2GLFrontend::MainLoop()
{
  while (!should_close)
  {
    SDL_Delay(1);
    console->StepFrame();

    // Grab framebuffer
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    Texture &display(console->GetPPU()->GetPatternTableLeftTexture());
    u8 *display_data = display.Data();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, display.GetWidth(), display.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, display_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    static char title[256];
    sprintf(title, "Frames: %u", console->GetFrameCount());
    SDL_SetWindowTitle(window, title);

    glViewport(0,0,1280, 720);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      //   ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        should_close = true;

      if (event.type == SDL_KEYDOWN)
        if (event.key.keysym.sym == SDLK_ESCAPE)
          should_close = true;
    }

    //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(0, 0);
    glTexCoord2f(0, 0);
    glVertex2f(0, 1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(1, 1);
    glVertex2f(1, 0);
    glEnd();

    //ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
}

SDL2GLFrontend::~SDL2GLFrontend()
{
}