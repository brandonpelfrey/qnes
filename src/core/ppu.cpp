#include "./ppu.h"
#include "core/bus.h"

PPU::PPU()
{
  scanline = 0;
  pixel_x = 0;

  PPUSTATUS = 0;
  PPUCTRL = 0;

  ppu_scroll_input_xy = 0;
}

u8 PPU::Read(u16 addr)
{
  if (addr == 0x2000)
  {
    return PPUCTRL;
  }
  if (addr == 0x2002)
  {
    u8 result = PPUSTATUS;

    VerticalBlank = 0; // Clear Vertical blank on PPUStatus read.
                       // TODO : clear address latch

    if (result)
      printf("Read to PPUSTatus: %02X\n", result);
    return result;
  }
  else
  {
    printf("Unimplemmented PPU read @ 0x%04X\n", addr);
    assert(0);
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
  else if (addr = 0x2005)
  {
    if (ppu_scroll_input_xy)
      scroll_x = val;
    else
      scroll_y = val;
    ppu_scroll_input_xy = !ppu_scroll_input_xy;
  }
  else if (addr == 0x4014)
  {
    // OAM DMA initiate
    bus->GetCPU()->InitiateOAMDMACounter();

    u16 page = (u16)(val) << 8;
    for (int i = 0; i < 256; ++i)
    {
      OAM_RAM[i & 0xFF] = bus->Read(page | ((i + OAMADDR) & 0xFF));
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
  if (scanline == FIRST_VERTICAL_BLANK_LINE && pixel_x == 1)
  {
    VerticalBlank = 1;
    printf("scanline %u -- PPUSTATUS = 0x%02X\n", scanline, PPUSTATUS);
  }

  // This is separated from when we enter Vertical Blank, because if a user does not
  // read PPUSTATUS to clear VerticalBlank flag, they can toggle GenerateNMI multiple
  // times and generate multiple NMIs. This code allows that to happen.
  if (VerticalBlank && GenerateNMIOnVBI && nmi_latch == 0)
  {
    nmi_latch = 1;
    bus->TriggerNMI();
  }

  if (scanline == PRE_RENDER_SCANLINE && pixel_x == 1)
  {
    VerticalBlank = 0;
    nmi_latch = 0; // Reset NMI latch
  }

  // Advance pixels/scanlines
  pixel_x++;
  if (pixel_x == 341)
  {
    pixel_x = 0;

    scanline++;

    if (scanline == PRE_RENDER_SCANLINE)
    {
      endFrameCallBack();
    }

    if (scanline == PRE_RENDER_SCANLINE + 1)
    {
      scanline = 0;
    }
  }
}