#include <cstdio>
#include "core/cartridge.h"
#include "core/console.h"

#include "frontend/SDL2GLFrontend.h"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("usage: %s [rom-file-path]", argv[0]);
    exit(1);
  }

  std::shared_ptr<Console> console = std::make_shared<Console>();
  console->LoadROM(argv[1]);
  console->HardReset();

  Frontend *frontend = new SDL2GLFrontend();
  frontend->SetConsole(console);
  frontend->MainLoop();
  return 0;
}