

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <unordered_map>
#include <thread>

#include "./SDL2GLFrontend.h"
#include "core/console.h"

#include "frontend/window_cpu.h"
#include "frontend/imgui_memory_editor.h"

const float vertices[] = {
    -1, -1, 0, 1,
    1, -1, 1, 1,
    1, 1, 1, 0,
    -1, 1, 0, 0};
GLuint vbo, vertexShader, fragmentShader, shaderProgram, vao;
GLint status;
GLuint shader_w, shader_h;

CPUWindow *cpu_window;
static MemoryEditor mem_edit_window;

const char *vertexSource = R"glsl(
    #version 150 core
    uniform float w;
    uniform float h;
    
    in vec2 position;
    in vec2 in_uv;
    out vec2 tex;

    void main()
    {
      tex = in_uv;
      gl_Position = vec4(position.x * w, position.y * h, 0.0, 1.0);
    }
)glsl";

const char *fragmentSource = R"glsl(
    #version 150 core
    uniform sampler2D texmap;
    in vec2 tex;
    out vec4 outColor;
    void main()
    {
        vec4 col = texture(texmap, tex);
        outColor = vec4(col.xyz, 1.0);
    }
)glsl";

void SDL2GLFrontend::imgui()
{
  ImGui::StyleColorsDark();

  static int call_number = 0;
  static float clear_color[3] = {.2, .3, .3};

  {
    ImGui::Begin("Basic ImGui Window");
    ImGui::Text("Current Frame is %u", call_number++);
    ImGui::ColorEdit3("Change Background Color", (float *)&clear_color);
    ImGui::End();
  }

  cpu_window->draw(false);

  mem_edit_window.DrawWindow("CPU RAM", console->GetBus()->GetRAMView(), 0x0800, 0);
}

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
#if __APPLE__
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
  {
    printf("Failed to initialize GLAD\n");
    exit(-1);
  }

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &framebuffer_texture);

  // VAO
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // VBO
  glGenBuffers(1, &vbo); // Generate 1 buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE)
  {
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    printf("Vertex shader compilation failed...\n%s\n", buffer);
    exit(-1);
  }

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE)
  {
    char buffer[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
    printf("Fragment shader compilation failed...\n%s\n", buffer);
    exit(-1);
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor");
  glLinkProgram(shaderProgram);

  glUseProgram(shaderProgram);
  shader_w = glGetUniformLocation(shaderProgram, "w");
  shader_h = glGetUniformLocation(shaderProgram, "h");

  const auto size_of_vertex = 4 * sizeof(GLfloat);
  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, size_of_vertex, 0);

  GLint texAttrib = glGetAttribLocation(shaderProgram, "in_uv");
  glEnableVertexAttribArray(texAttrib);
  glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, size_of_vertex, (void *)(2 * sizeof(GLfloat)));

  cpu_window = new CPUWindow(console, nullptr, nullptr);
}

void SDL2GLFrontend::MainLoop()
{
  cpu_window->SetConsole(console);

  ImGuiContext imgui_context;

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
      ImGui_ImplSDL2_ProcessEvent(&event);

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

      for (const auto &binding : key_bindings)
        if (event.key.keysym.sym == binding.first)
        {
          if (event.type == SDL_KEYDOWN)
            console->GetControllers()->SetButtonPressed(0, binding.second, 1);
          else if (event.type == SDL_KEYUP)
            console->GetControllers()->SetButtonPressed(0, binding.second, 0);
        }
    }

    // This is not equal to window size in high DPI mode, so need to use GL_GetDrawableSize.
    int gl_drawable_width, gl_drawable_height;
    SDL_GL_GetDrawableSize(window, &gl_drawable_width, &gl_drawable_height);
    glViewport(0, 0, gl_drawable_width, gl_drawable_height);

    glUseProgram(0);
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

    //glEnable(GL_TEXTURE_2D);
    //glBindTexture(GL_TEXTURE_2D, framebuffer_texture);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1, 1, 1);

    glUseProgram(shaderProgram);
    glUniform1f(shader_w, w);
    glUniform1f(shader_h, h);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    /*
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
    */

    imgui_context.StartImGuiFrame();
    imgui();
    imgui_context.EndImGuiFrame();

    SDL_GL_SwapWindow(window);
  }
}
