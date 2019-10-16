#include <cstdio>
#include "core/cartridge.h"
#include "core/console.h"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("usage: %s [rom-file-path]", argv[0]);
    exit(1);
  }

  Cartridge::LoadRomFile(argv[1]);

  Console console;
  console.Test1();
  return 0;
}