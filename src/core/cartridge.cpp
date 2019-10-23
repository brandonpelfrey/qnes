#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "core/cartridge.h"

#include "mappers/mapper_NROM.h"

Cartridge *Cartridge::LoadRomFile(const char *path)
{
  FILE *romFile = fopen(path, "r");

  // iNES File Format
  // 1. 16-byte Header
  // 2. Trainer, if present (0 or 512 bytes)
  // 3. PRG ROM Data (16k * x bytes)
  // 4. CHR ROM data, if present (8192 * y bytes)
  // ... (PlayChoice stuff, but don't care about that)

  u8 header[16];
  fread(header, sizeof(u8), 16, romFile);

  // Confirm this is an iNES File first
  static u8 iNESMagicHeader[4] = {0x4E, 0x45, 0x53, 0x1A};
  if (memcmp(header, iNESMagicHeader, 4) != 0)
  {
    printf("File '%s' is missing the required iNES Header.\n", path);
    exit(1);
  }

  bool romHasTrainer = header[6] & 4;
  if (romHasTrainer)
  {
    printf("Rom has a built-in trainer, which is not handled.\n");
    exit(1);
  }

  CartridgeDescription description;
  description.PRG_ROM_16KB_Multiple = header[4];
  description.CHR_ROM_8KB_Multiple = header[5];
  bool HardwiredMirroringModeIsVertical;
  description.HardwiredMirroringModeIsVertical = header[6] & 1;
  description.HasBatteryBackedRAM = header[6] & 2;
  description.IgnoreMirroringControl = header[6] & 8;
  description.MapperNumber = (header[6] >> 4) | (header[7] & 0xF0);

  // Print the description
  printf("Loaded rom file '%s' (Mapper %d, %uKB PRG-ROM, %uKB CHR-ROM",
         path,
         description.MapperNumber,
         16 * description.PRG_ROM_16KB_Multiple,
         8 * description.CHR_ROM_8KB_Multiple);

  if (description.HasBatteryBackedRAM)
    printf(", Battery-Backed RAM");

  printf(")\n");

  Cartridge *result;
  switch (description.MapperNumber)
  {
  case 0:
    result = new Mapper_NROM();
    break;

  default:
    printf("Unsupported Mapper %u.\n", description.MapperNumber);
    exit(1);
    break;
  }

  result->description = description;

  int prg_rom_bytes = 0x4000 * description.PRG_ROM_16KB_Multiple;
  result->PRG_ROM = new u8[prg_rom_bytes];
  fread(result->PRG_ROM, sizeof(u8), prg_rom_bytes, romFile);

  int chr_rom_bytes = 0x2000 * description.CHR_ROM_8KB_Multiple;
  result->CHR_ROM = new u8[chr_rom_bytes];
  fread(result->CHR_ROM, sizeof(u8), chr_rom_bytes, romFile);

  fclose(romFile);

  return result;
}