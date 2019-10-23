#include "./ppu.h"
#include "core/bus.h"
#include "core/cartridge.h"

// Palettes
// https://wiki.nesdev.com/w/index.php/PPU_palettes
// To get this format : hexdump -e '192/1 "0x%02x," "\n"'  PALFILE
u8 PALETTE_BYTES[64 * 3] = {0x60, 0x60, 0x60, 0x00, 0x00, 0x78, 0x14, 0x00, 0x80, 0x2c, 0x00, 0x6e, 0x4a, 0x00, 0x4e, 0x6c, 0x00, 0x18, 0x5a, 0x03, 0x02, 0x51, 0x18, 0x00, 0x34, 0x24, 0x00, 0x00, 0x34, 0x00, 0x00, 0x32, 0x00, 0x00, 0x34, 0x20, 0x00, 0x2c, 0x78, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0xc4, 0xc4, 0xc4, 0x00, 0x58, 0xde, 0x30, 0x1f, 0xfc, 0x7f, 0x14, 0xe0, 0xa8, 0x00, 0xb0, 0xc0, 0x06, 0x5c, 0xc0, 0x2b, 0x0e, 0xa6, 0x40, 0x10, 0x6f, 0x61, 0x00, 0x30, 0x80, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x7c, 0x3c, 0x00, 0x6e, 0x84, 0x14, 0x14, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0xf0, 0xf0, 0xf0, 0x4c, 0xaa, 0xff, 0x6f, 0x73, 0xf5, 0xb0, 0x70, 0xff, 0xda, 0x5a, 0xff, 0xf0, 0x60, 0xc0, 0xf8, 0x83, 0x6d, 0xd0, 0x90, 0x30, 0xd4, 0xc0, 0x30, 0x66, 0xd0, 0x00, 0x26, 0xdd, 0x1a, 0x2e, 0xc8, 0x66, 0x34, 0xc2, 0xbe, 0x54, 0x54, 0x54, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xff, 0xff, 0xff, 0xb6, 0xda, 0xff, 0xc8, 0xca, 0xff, 0xda, 0xc2, 0xff, 0xf0, 0xbe, 0xff, 0xfc, 0xbc, 0xee, 0xff, 0xd0, 0xb4, 0xff, 0xda, 0x90, 0xec, 0xec, 0x92, 0xdc, 0xf6, 0x9e, 0xb8, 0xff, 0xa2, 0xae, 0xea, 0xbe, 0x9e, 0xef, 0xef, 0xbe, 0xbe, 0xbe, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

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
  frame_buffer.Resize(WIDTH, HEIGHT);
  pattern_left.Resize(128, 128);
  pattern_right.Resize(128, 128);
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
    if ((vram_addr & 0xFF00) != 0x3F00) //
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
    bool is_0_to_1_nmigen = ((PPUCTRL & 7) == 0) && (val & 7);
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
      //printf("ADDR = 0x%04X\n", vram_addr);
    }
  }
  else if (addr == 0x2007) // PPUDATA
  {
    // Write the value currently pointed at by the internal address latch,
    // then increment based on bit 2 of PPUCTRL

    vram[vram_addr] = val;
    printf("PPU Data Write 0x%02X -> 0x%04X\n", val, vram_addr);
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
  //const u16 FIRST_VISIBLE_SCANLINE = 0;
  //const u16 LAST_VISIBLE_SCANLINE = 239;
  //const u16 POST_RENDER_SCANLINE = 240;
  const u16 FIRST_VERTICAL_BLANK_LINE = 241;
  //const u16 LAST_VERTICAL_BLANK_LINE = 260;

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
      render_pattern_tables();
      endFrameCallBack();
    }

    if (pixel_y == PRE_RENDER_SCANLINE + 1)
    {
      pixel_y = 0;
    }
  }
}

u8 PPU::ppuRead(u16 addr)
{
  u8 val;
  if (cart->PPURead(addr, val))
  {
  }
  else if (addr >= 0x0000 && addr < 0x2000)
  {
    return vram[addr];
  }
  else if (addr < 0x3F00)
  {
    u16 effective = addr & 0x2FFF;
    bool is_horizontal = cart->GetDescription().HardwiredMirroringModeIsVertical;
    if (is_horizontal)
      effective &= addr < 0x2800 ? 0x23FF : 0x2BFF;
    else
      effective &= 0x27FF;
    printf("NT Lookup @ 0x%04X -> 0x%02X\n", effective, vram[effective]);
    return vram[effective];
  }
  else if (addr < 0x4000)
  {
    u16 effective = 0x3F00 | (addr & 0x00FF);
    return vram[effective];
  }
}

void PPU::render_pattern_tables()
{
  // Pattern table entries
  // DCBA98 76543210
  // ---------------
  // 0HRRRR CCCCPTTT
  // |||||| |||||+++- T: Fine Y offset, the row number within a tile
  // |||||| ||||+---- P: Bit plane (0: "lower"; 1: "upper")
  // |||||| ++++----- C: Tile column
  // ||++++---------- R: Tile row
  // |+-------------- H: Half of sprite table (0: "left"; 1: "right")
  // +--------------- 0: Pattern table is at $0000-$1FFF

  Texture *pattern_textures[2] = {&pattern_left, &pattern_right};

  for (int left_right = 0; left_right < 2; ++left_right)
  {
    u8 *pattern_texture_data = pattern_textures[left_right]->Data();

    for (int j = 0; j < 128; ++j)
    {
      u16 tile_row = j / 8;
      u16 fine_y = j & 7;

      for (int i = 0; i < 128; ++i)
      {
        u16 tile_col = i / 8;
        u16 fine_x = 7 - (i & 7);

        auto chrrom = [&](u16 addr) -> u8 { u8 val; return bus->GetCartridge()->PPURead(addr, val)? val : 0; };

        u8 lo_bit = (chrrom((left_right << 12) | (tile_row << 8) | (tile_col << 4) | 0b0000 | fine_y) >> fine_x) & 1;
        u8 hi_bit = (chrrom((left_right << 12) | (tile_row << 8) | (tile_col << 4) | 0b1000 | fine_y) >> fine_x) & 1;

        u8 pix_color = lo_bit | (hi_bit << 1);
        u8 palette_number = 0;
        u8 color_index = vram[pix_color == 0 ? 0x3F00 : 0x3F01 + 4 * palette_number + pix_color];

        u8 r = PALETTE_BYTES[3 * color_index + 0];
        u8 g = PALETTE_BYTES[3 * color_index + 1];
        u8 b = PALETTE_BYTES[3 * color_index + 2];

        u16 ptr_base = 3 * (j * 128 + i);
        pattern_texture_data[ptr_base + 0] = r;
        pattern_texture_data[ptr_base + 1] = g;
        pattern_texture_data[ptr_base + 2] = b;
      }
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

  ////////////////////////////////////////////////////

  // Get background color
  u16 offset_x = pixel_x + scroll_x;
  u16 offset_y = pixel_y + scroll_y;

  u8 tile_x = offset_x / 8;
  u8 fine_x = offset_x & 7;

  u8 tile_y = offset_y / 8;
  u8 fine_y = offset_y & 7;

  u8 lo_bit = (ppuRead((1 << 12) | (tile_y << 8) | (tile_x << 4) | 0b0000 | fine_y) >> fine_x) & 1;
  u8 hi_bit = (ppuRead((1 << 12) | (tile_y << 8) | (tile_x << 4) | 0b1000 | fine_y) >> fine_x) & 1;
  
  val = lo_bit | (hi_bit << 1);
  val = val ? 255 : 0;

  // Get sprite color

  // Combine

  pixels[3 * pixel_num] = val;
  pixels[3 * pixel_num + 1] = val;
  pixels[3 * pixel_num + 2] = val;
}