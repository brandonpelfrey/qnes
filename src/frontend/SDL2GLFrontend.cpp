#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <unordered_map>
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

    if (!console->GetCPU()->IsPaused())
      console->StepFrame();

    // Grab framebuffer
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    //Texture &display(console->GetPPU()->GetPatternTableLeftTexture());
    Texture &display(console->GetPPU()->GetFrameBufferTexture());
    u8 *display_data = display.Data();
    //printf("%u x %u\n", display.GetWidth(), display.GetHeight());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, display.GetWidth(), display.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, display_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    static char title[256];
    sprintf(title, "Frames: %u", console->GetFrameCount());
    SDL_SetWindowTitle(window, title);

    std::unordered_map<unsigned, Controllers::ButtonMask> key_bindings = {
        {SDLK_RETURN, Controllers::Start},
        {SDLK_RSHIFT, Controllers::Select},
        {SDLK_z, Controllers::A},
        {SDLK_x, Controllers::B},
        {SDLK_UP, Controllers::Up},
        {SDLK_DOWN, Controllers::Down},
        {SDLK_LEFT, Controllers::Left},
        {SDLK_RIGHT, Controllers::Right},
    };

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
        should_close = true;

      if (event.type == SDL_KEYDOWN)
      {
        switch (event.key.keysym.sym)
        {
        case SDLK_s:
          console->StepCPU();
          break;

        case SDLK_ESCAPE:
          should_close = true;
          break;
        }
      }

      if (event.type == SDL_KEYDOWN)
      {
        for (const auto &binding : key_bindings)
          if (event.key.keysym.sym == binding.first)
            console->GetControllers()->SetButtonPressed(0, binding.second, 1);
      }

      if (event.type == SDL_KEYUP)
      {
        for (const auto &binding : key_bindings)
          if (event.key.keysym.sym == binding.first)
            console->GetControllers()->SetButtonPressed(0, binding.second, 0);
      }
    }

    // This is not equal to window size in high DPI mode, so need to use GL_GetDrawableSize.
    int gl_drawable_width, gl_drawable_height;
    SDL_GL_GetDrawableSize(window, &gl_drawable_width, &gl_drawable_height);
    glViewport(0, 0, gl_drawable_width, gl_drawable_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, 1, -1);

    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Combine the window aspect ratio and aspect ratio of the texture to be drawn
    // to produce width and height variables in [0,1] which will fit inside
    // a [-1,1] x [-1,1] square.
    float window_aspect = (float)gl_drawable_width / (float)gl_drawable_height;
    float texture_aspect = (float)display.GetWidth() / (float)display.GetHeight();
    window_aspect /= texture_aspect;
    float w = std::min(1.f, 1.f / window_aspect);
    float h = std::min(1.f, 1.f * window_aspect);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-w, -h);

    glTexCoord2f(1, 1);
    glVertex2f(w, -h);

    glTexCoord2f(1, 0);
    glVertex2f(w, h);

    glTexCoord2f(0, 0);
    glVertex2f(-w, h);
    glEnd();

    //ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
}

SDL2GLFrontend::~SDL2GLFrontend()
{
}