#include "./ppu.h"
#include "core/bus.h"

// NES Framebuffer/Screen size
const int WIDTH = 256;
const int HEIGHT = 240;

PPU::PPU()
{
  pixel_y = 0;
  pixel_x = 0;

  PPUSTATUS = 0;
  PPUCTRL = 0;

  address_latch = 0;
  vram = new u8[0x4000];
  frame_buffer = Texture(WIDTH, HEIGHT);
}

PPU::~PPU()
{
  delete[] vram;
}

u8 PPU::Read(u16 addr)
{
  if (addr == 0x2000)
  {
    return PPUCTRL;
  }
  else if (addr == 0x2002)
  {
    u8 result = PPUSTATUS;

    VerticalBlank = 0; // Clear Vertical blank on PPUStatus read.
    address_latch = 0;

    if (result)
      printf("Read to PPUSTatus: %02X\n", result);
    return result;
  }
  else if (addr == 0x2007)
  {

    // Read @ vram_addr pointer
    if (vram_addr & 0xFF00 != 0x3F00) //
    {
      //u8 return_value = PPU_DATA_read_buffer;
      //PPU_DATA_read_buffer = VRAM[vram_addr];
      //return return_value;
    }
    else
    {
    }

    assert(0);
  }
  else
  {
    printf("Unimplemmented PPU read @ 0x%04X\n", addr);
    //assert(0);
    return 0;
  }
}

void PPU::Write(u16 addr, u8 val)
{
  if (addr == 0x2000)
  {
    bool is_0_to_1_nmigen = (PPUCTRL & 7 == 0) && (val & 7);
    if (VerticalBlank && GenerateNMIOnVBI && is_0_to_1_nmigen)
    {
      nmi_latch = 1;
      bus->TriggerNMI();
    }

    PPUCTRL = val;
  }
  else if (addr == 0x2001)
  {
    PPUMASK = val;
  }
  else if (addr == 0x2003)
  {
    OAMADDR = val;
  }
  else if (addr == 0x2005)
  {
    if (address_latch == 0)
    {
      scroll_x = val;
      address_latch = 1;
    }
    else
    {
      scroll_y = val;
      address_latch = 0;
    }
  }
  else if (addr == 0x2006) // PPUADDR
  {
    if (address_latch == 0)
    {
      vram_addr = (u16)(val & 0x3F) << 8;
      address_latch = 1;
    }
    else
    {
      vram_addr = vram_addr | (val & 0xFF);
      address_latch = 0;
      printf("ADDR = 0x%04X\n", vram_addr);
    }
  }
  else if (addr == 0x2007) // PPUDATA
  {
    // Write the value currently pointed at by the internal address latch,
    // then increment based on bit 2 of PPUCTRL

    vram[vram_addr] = val;
    //printf("PPU Data 0x%02X -> 0x%04X\n", val, vram_addr);
    u8 addr_increment = VRAMAddressIncrement ? 32 : 1;
    vram_addr = (vram_addr + addr_increment) & 0x3FFF;
  }
  else if (addr == 0x4014)
  {
    // OAM DMA initiate
    bus->GetCPU()->InitiateOAMDMACounter();

    u16 page = (u16)(val) << 8;
    for (int i = 0; i < 256; ++i)
    {
      OAM_RAM[i & 0xFF] = bus->Read(page | ((OAMADDR)&0xFF));
      //vram[i & 0xFF] = OAM_RAM[i & 0xFF];
      OAMADDR++;
    }

    // TODO : This is not-cycle accurate.
  }
  else
  {
    printf("Unimplemmented PPU write @ 0x%04X\n", addr);
    assert(0);
  }
}

void PPU::Clock()
{
  const u16 PRE_RENDER_SCANLINE = 261;
  const u16 FIRST_VISIBLE_SCANLINE = 0;
  const u16 LAST_VISIBLE_SCANLINE = 239;
  const u16 POST_RENDER_SCANLINE = 240;
  const u16 FIRST_VERTICAL_BLANK_LINE = 241;
  const u16 LAST_VERTICAL_BLANK_LINE = 260;

  // The second tick of scanline 241 sets VBlank flag, and also triggers NMI
  if (pixel_y == FIRST_VERTICAL_BLANK_LINE && pixel_x == 1)
  {
    VerticalBlank = 1;
    printf("scanline %u -- PPUSTATUS = 0x%02X\n", pixel_y, PPUSTATUS);
  }

  // This is separated from when we enter Vertical Blank, because if a user does not
  // read PPUSTATUS to clear VerticalBlank flag, they can toggle GenerateNMI multiple
  // times and generate multiple NMIs. This code allows that to happen.
  if (VerticalBlank && GenerateNMIOnVBI && nmi_latch == 0)
  {
    nmi_latch = 1;
    bus->TriggerNMI();
  }

  if (pixel_y == PRE_RENDER_SCANLINE && pixel_x == 1)
  {
    VerticalBlank = 0;
    nmi_latch = 0; // Reset NMI latch
  }

  // TODO : Actual render pixel
  render_pixel();

  // Advance pixels/scanlines
  pixel_x++;
  if (pixel_x == 341)
  {
    pixel_x = 0;

    pixel_y++;

    if (pixel_y == PRE_RENDER_SCANLINE)
    {
      endFrameCallBack();
    }

    if (pixel_y == PRE_RENDER_SCANLINE + 1)
    {
      pixel_y = 0;
    }
  }
}

void PPU::render_pixel()
{
  u8 val = 0;

  u8 *pixels = frame_buffer.Data();
  if (pixel_x >= WIDTH || pixel_y >= HEIGHT)
    return;

  u32 pixel_num = WIDTH * pixel_y + pixel_x;
  u32 q = pixel_y * WIDTH + pixel_x;
  q /= 2;

  if (q < 0x2000)
    bus->GetCartridge()->PPURead(q, val);
  else if (q < 0x3000)
    val = vram[q - 0x2000];
  else
    val = 0;

  val = val ? 255 : 0;

  pixels[3 * pixel_num] = val;
  pixels[3 * pixel_num + 1] = val;
  pixels[3 * pixel_num + 2] = val;
}