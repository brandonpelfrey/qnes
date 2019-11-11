#include "mappers/Mapper_001.h"
#include "core/types.h"

Mapper_001::Mapper_001(CartridgeDescription description) noexcept
    : description(description)
{
  PRG_RAM = new u8[0x2000];

  PRGSelect = 0;
  ControlRegister = 0b01111;
  updateOffsets();
}

Mapper_001::~Mapper_001()
{
  delete[] PRG_RAM;
}

// MMC1
bool Mapper_001::CPURead(u16 addr, u8 &val)
{
  // CPU $6000-$7FFF: 8 KB PRG RAM bank, (optional)
  // CPU $8000-$BFFF: 16 KB PRG ROM bank, either switchable or fixed to the first bank
  // CPU $C000-$FFFF: 16 KB PRG ROM bank, either fixed to the last bank or switchable

  if (addr < 0x6000)
    return false;

  if (addr < 0x8000)
  {
    int offset = addr - 0x6000;
    val = PRG_RAM[offset];
  }
  else
  {
    addr -= 0x8000;
    int bank = addr / 0x4000;
    int offset = addr % 0x4000;
    val = PRG_ROM[PRGOffsets[bank] + offset];
  }

  return true;
}

bool Mapper_001::CPUWrite(u16 addr, u8 val)
{
  if (addr < 0x6000)
  {
    // A couple of games have tried to write here. It's undefined behavior as far as I know.
    // TODO ???
    return false;
  }

  // First handle RAM. Everything else uses the shift register
  if (addr < 0x8000)
  {
    int offset = addr - 0x6000;
    PRG_RAM[offset] = val;
    return true;
  }

  // Writing with bit 7 set, this write just resets the shift register
  if (val & 0x80)
  {
    shift_register = 0x00;
    ControlRegister |= 0x0C;
    shift_register_write_counter = 0;

    updateOffsets();
    return true;
  }

  // We're shifting in a bit to the shift register..
  shift_register >>= 1;
  shift_register |= (u8)((val & 1) << 4);
  shift_register_write_counter++;

  // Should we do an actual write?
  if (shift_register_write_counter == 5)
  {
    shift_register &= 0x1F;

    if (addr < 0xA000)
    {
      ControlRegister = shift_register;

      // TODO
      switch (ControlRegister & 3)
      {
      case 0:
        //cartridge.NametableMirroring = NametableMirroringMode.OneScreenLowBank;
        break;
      case 1:
        //cartridge.NametableMirroring = NametableMirroringMode.OneScreenHighBank;
        break;
      case 2:
        description.HardwiredMirroringModeIsVertical = 1;
        break;
      case 3:
        description.HardwiredMirroringModeIsVertical = 0;
        break;
      }
    }
    else if (addr < 0xC000)
    {
      CHR0Select = shift_register;
    }
    else if (addr < 0xE000)
    {
      CHR1Select = shift_register;
    }
    else
    {
      //Console.WriteLine("Switched to PRG {0}", shift_register & 0x0F);
      PRGSelect = shift_register & 0x0F; // Ignore PRG RAM Enable (TODO?)
    }

    // Update offsets
    updateOffsets();

    shift_register = 0x00;
    shift_register_write_counter = 0;
  }

  return true;
}

bool Mapper_001::PPURead(u16 addr, u8 &val)
{
  // PPU $0000-$0FFF: 4 KB switchable CHR bank
  // PPU $1000-$1FFF: 4 KB switchable CHR bank

  if (addr < 0x2000)
  {
    int bank = addr / 0x1000;
    val = CHR_ROM[CHROffsets[bank] | (addr & 0xFFF)];
    return true;
  }
  return false;
}

void Mapper_001::updateOffsets()
{
  // CHR0 and CHR1
  if ((ControlRegister & 0x10) == 0)
  {
    // Switch 8KB at a time
    CHROffsets[0] = 0x1000 * (CHR0Select & 0x1E);
    CHROffsets[1] = CHROffsets[0] + 0x1000;
  }
  else
  {
    // Switch two separate 4KB banks
    CHROffsets[0] = 0x1000 * (CHR0Select & 0x1F);
    CHROffsets[1] = 0x1000 * (CHR1Select & 0x1F);
  }

  // PRG Select
  if ((ControlRegister & 0x08) == 0)
  {
    PRGOffsets[0] = 0x4000 * (PRGSelect & 0x0E);
    PRGOffsets[1] = PRGOffsets[0] + 0x4000;
  }
  else
  {
    // 16KB PRG Switching. There are two possible modes for switching.

    // If Control's Bit 2 is set, PRG is swapped at 0xC0000
    if ((ControlRegister & 0x04) == 0)
    {
      PRGOffsets[0] = 0;
      PRGOffsets[1] = 0x4000 * PRGSelect;
    }
    else
    {
      PRGOffsets[0] = 0x4000 * PRGSelect;
      PRGOffsets[1] = 0x4000 * (description.PRG_ROM_16KB_Multiple - 1);
    }
  }
}

bool Mapper_001::PPUWrite(u16 addr, u8 val)
{
  return false;
}
