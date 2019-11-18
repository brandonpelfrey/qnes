#include "./ppu.h"
#include "core/bus.h"
#include "core/cartridge.h"
#include <cassert>
#include <cstring>

// Palettes
// https://wiki.nesdev.com/w/index.php/PPU_palettes
// To get this format from a pallete file, use:
//   hexdump -e '192/1 "0x%02x," "\n"'  PALFILE
u8 PALETTE_BYTES[64 * 3] = {
    0x60, 0x60, 0x60, 0x00, 0x00, 0x78, 0x14, 0x00, 0x80, 0x2c, 0x00,
    0x6e, 0x4a, 0x00, 0x4e, 0x6c, 0x00, 0x18, 0x5a, 0x03, 0x02, 0x51,
    0x18, 0x00, 0x34, 0x24, 0x00, 0x00, 0x34, 0x00, 0x00, 0x32, 0x00,
    0x00, 0x34, 0x20, 0x00, 0x2c, 0x78, 0x00, 0x00, 0x00, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0xc4, 0xc4, 0xc4, 0x00, 0x58, 0xde, 0x30,
    0x1f, 0xfc, 0x7f, 0x14, 0xe0, 0xa8, 0x00, 0xb0, 0xc0, 0x06, 0x5c,
    0xc0, 0x2b, 0x0e, 0xa6, 0x40, 0x10, 0x6f, 0x61, 0x00, 0x30, 0x80,
    0x00, 0x00, 0x7c, 0x00, 0x00, 0x7c, 0x3c, 0x00, 0x6e, 0x84, 0x14,
    0x14, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0xf0, 0xf0, 0xf0,
    0x4c, 0xaa, 0xff, 0x6f, 0x73, 0xf5, 0xb0, 0x70, 0xff, 0xda, 0x5a,
    0xff, 0xf0, 0x60, 0xc0, 0xf8, 0x83, 0x6d, 0xd0, 0x90, 0x30, 0xd4,
    0xc0, 0x30, 0x66, 0xd0, 0x00, 0x26, 0xdd, 0x1a, 0x2e, 0xc8, 0x66,
    0x34, 0xc2, 0xbe, 0x54, 0x54, 0x54, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0xff, 0xff, 0xff, 0xb6, 0xda, 0xff, 0xc8, 0xca, 0xff, 0xda,
    0xc2, 0xff, 0xf0, 0xbe, 0xff, 0xfc, 0xbc, 0xee, 0xff, 0xd0, 0xb4,
    0xff, 0xda, 0x90, 0xec, 0xec, 0x92, 0xdc, 0xf6, 0x9e, 0xb8, 0xff,
    0xa2, 0xae, 0xea, 0xbe, 0x9e, 0xef, 0xef, 0xbe, 0xbe, 0xbe, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08};

// NES Framebuffer/Screen size
const int WIDTH = 256;
const int HEIGHT = 240;

PPU::PPU()
{
  pixel_y = 0;
  pixel_x = 0;
  nmi_latch = address_latch = PPU_DATA_read_buffer = 0;
  OAMADDR = 0;

  PPUSTATUS = 0;
  PPUCTRL = 0;
  PPUMASK = 0;
  scroll_x = 0;
  scroll_y = 0;
  vram_addr = 0;

  address_latch = 0;
  vram = new u8[0x4000];
  OAM_RAM = new u8[64 * 4];
  memset(vram, 0, 0x4000);
  memset(OAM_RAM, 0, 64 * 4);

  frame_buffer.Resize(WIDTH, HEIGHT);
  pattern_left.Resize(128, 128);
  pattern_right.Resize(128, 128);

  // This would ideally by 2*(256,240), but to simplify life for
  // OpenGL, making this a power-of-two texture
  nametables.Resize(256 * 2, 256 * 2);
}

PPU::~PPU()
{
  delete[] vram;
}

void PPU::GetState(PPURegisterState *state)
{
  state->address_latch = address_latch;
  state->nmi_latch = nmi_latch;

  state->OAMADDR = OAMADDR;
  state->vram_addr = vram_addr;

  state->PPUSTATUS = PPUSTATUS;
  state->PPUCTRL = PPUCTRL;
  state->PPUMASK = PPUMASK;

  state->scroll_x = scroll_x;
  state->scroll_y = scroll_y;
  state->pixel_x = pixel_x;
  state->pixel_y = pixel_y;
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

    // Clear Vertical blank and latches on PPUStatus read.
    VerticalBlank = 0;
    address_latch = 0;

    return result;
  }
  else if (addr == 0x2007)
  {
    // Read @ vram_addr pointer

    // For most values, the value returned is actually a buffer,
    // so you'd need to read twice to get the correct value
    u8 return_value = PPU_DATA_read_buffer;
    PPU_DATA_read_buffer = ppuRead(vram_addr & 0x3FFF);

    // However, if the read is to a pallete, the data is returned immediately.
    if (vram_addr >= 0x3F00)
      return_value = PPU_DATA_read_buffer;

    // Auto-increment the internal VRAM pointer by an amount
    // specified in PPUCTRL
    u8 addr_increment = (PPUCTRL & 0b100) ? 32 : 1;
    vram_addr = (vram_addr + addr_increment) & 0x3FFF;
    return return_value;
  }
  else
  {
    printf("Unimplemmented PPU read @ 0x%04X\n", addr);
    return 0;
  }
}

void PPU::Write(u16 addr, u8 val)
{
  if (addr == 0x2000)
  {
    // There's a special case handled here where NMI disabled ->
    // enabled during VBlank should immediately cause an NMI.
    bool is_0_to_1_nmigen = ((PPUCTRL & 0x80) == 0) && (val & 0x80);
    if (VerticalBlank && (nmi_latch == 0) && is_0_to_1_nmigen)
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
  else if (addr == 0x2004)
  {
    OAM_RAM[OAMADDR] = val;
    OAMADDR = (OAMADDR + 1) & 0xFF;
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
      // printf("[SL %d] Changed Scroll to (%d,%d)\n", pixel_y, scroll_x, scroll_y); // SMB1 debugging...
    }
  }
  else if (addr == 0x2006) // PPUADDR
  {
    if (address_latch == 0)
    {
      vram_addr = (val & 0x7F) << 8;
      address_latch = 1;
    }
    else
    {
      vram_addr = vram_addr | val;
      address_latch = 0;
    }
  }
  else if (addr == 0x2007) // PPUDATA
  {
    // Write the value currently pointed at by the internal address latch,
    // then increment based on bit 2 of PPUCTRL

    vram_addr &= 0x3FFF;

    /*
    // TODO
    // Writes to mirrored palletes' background colors
    // are supposed to write into the first four,
    // but for some reason this is screwing up the
    // background colors in many places.
    if (vram_addr == 0x3F10) vram_addr = 0x3F00;
    if (vram_addr == 0x3F14) vram_addr = 0x3F04;
    if (vram_addr == 0x3F18) vram_addr = 0x3F08;
    if (vram_addr == 0x3F1C) vram_addr = 0x3F0C;
    */

    vram[NametableMirroring(vram_addr)] = val;
    u8 addr_increment = (PPUCTRL & 0b100) ? 32 : 1;
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
      OAMADDR++;
    }
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
    printf("scanline %u -- PPUSTATUS = 0x%02X, PPUCTRL = 0x%02X\n", pixel_y, PPUSTATUS, PPUCTRL);
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
    SpriteZeroHit = 0;
    nmi_latch = 0; // Reset NMI latch
  }

  render_pixel();

  // Advance pixels/scanlines
  pixel_x++;
  if (pixel_x == 341)
  {
    pixel_x = 0;
    pixel_y++;

    if (pixel_y == 33)
    {
      printf("scanline %d -- S0 :: %02X %02X %02X %02X --- SX/SY :: %d %d\n", pixel_y, OAM_RAM[0], OAM_RAM[1], OAM_RAM[2], OAM_RAM[3], scroll_x, scroll_y);
    }

    if (pixel_y == 2)
    {
      render_pattern_tables();
      render_nametables();
    }

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

void PPU::render_nametables()
{
  u8 *nt_data = nametables.Data();

  // PPUCTRL marks whether the background tiles from from the 'left' or 'right' pattern tables.
  const u16 bg_pattern_base = BGPatternTableAddress ? 0x1000 : 0x0000;

  // We fill up the entire texture, which is comprised of data from all four nametables
  // Because the data is arranged in chunks of 8 horizontal pixels in a row, we'll fetch
  // data for the next 8 pixels, and draw that into our texture before going onto the
  // next 8 pixels.
  for (int j = 0; j < 240 * 2; j += 1)
    for (int i = 0; i < 256 * 2; i += 8)
    {
      // (i,j) position within the full space of the nametables.

      int nametable_tile_x = (i / 8) % 32;
      int nametable_tile_y = (j / 8) % 30;

      const u8 which_nametable = (i / 256) + 2 * (j / 240);
      u16 nametable_start = 0x2000 + 0x400 * which_nametable;
      u16 nt_byte_addr = nametable_start + 32 * nametable_tile_y + nametable_tile_x; //(nametable_tile_y * 32 + nametable_tile_x);
      u16 pattern_table_index = ppuRead(nt_byte_addr);

      u8 lo_bits = ppuRead(bg_pattern_base | (pattern_table_index << 4) | 0b0000 | (j & 7));
      u8 hi_bits = ppuRead(bg_pattern_base | (pattern_table_index << 4) | 0b1000 | (j & 7));

      u8 attribute_index = (nametable_tile_y / 4) * 8 + (nametable_tile_x / 4);
      u8 attribute_byte = ppuRead(nametable_start + 0x3C0 + attribute_index);
      u8 attribute_bits = ((i % 32) / 16) * 2 + ((j % 32) / 16) * 4;
      u8 bg_pal_numb = (attribute_byte >> attribute_bits) & 0b11;

      for (int fine_x = i; fine_x < i + 8; ++fine_x)
      {
        u8 lo_bit = (lo_bits >> (7 - fine_x & 7)) & 1;
        u8 hi_bit = (hi_bits >> (7 - fine_x & 7)) & 1;
        u8 bg_color_index = lo_bit | (hi_bit << 1);

        u8 master_color_index = ppuRead(0x3F00 + 4 * bg_pal_numb + bg_color_index);

        u8 r = PALETTE_BYTES[3 * master_color_index + 0];
        u8 g = PALETTE_BYTES[3 * master_color_index + 1];
        u8 b = PALETTE_BYTES[3 * master_color_index + 2];

        // Outline the 4 screens
        if (fine_x == 256 || j == 240)
        {
          r = g = b = 255;
        }

        const int pixel_num = 512 * j + fine_x;
        nt_data[3 * pixel_num + 0] = r;
        nt_data[3 * pixel_num + 1] = g;
        nt_data[3 * pixel_num + 2] = b;
      }
    }

  // Draw the window into the namespaces
  const int ppu_scroll_x = scroll_x + ((PPUCTRL >> 0) & 1 ? 256 : 0);
  const int ppu_scroll_y = scroll_y + ((PPUCTRL >> 1) & 1 ? 240 : 0);

  for (int i = 0; i < 256; ++i)
    for (int j = 0; j < 240; ++j)
    {
      bool is_border = (i == 0) || (j == 0) || (i == 255) || (j == 239);
      if (is_border)
      {
        const int i_wrapped = (i + ppu_scroll_x) % (256 * 2);
        const int j_wrapped = (j + ppu_scroll_y) % (240 * 2);
        const int pixel_num = 512 * j_wrapped + i_wrapped;

        nt_data[3 * pixel_num + 0] = 255;
        nt_data[3 * pixel_num + 1] = 255;
        nt_data[3 * pixel_num + 2] = 255;
      }
    }
}

u16 PPU::NametableMirroring(u16 addr)
{
  if (addr >= 0x2000 && addr < 0x3F00)
  {
    bool is_horizontal = cart->GetDescription().HardwiredMirroringModeIsVertical;
    if (is_horizontal)
      return (addr < 0x2800) ? (0x2000 | (addr & 0x3FF)) : (0x2800 | (addr & 0x3FF));
    else
      return addr & 0xF7FF;
  }
  else
    return addr;
}

u8 PPU::ppuRead(u16 addr)
{
  u8 val;
  if (cart->PPURead(addr, val))
  {
    return val;
  }
  else if (addr >= 0x0000 && addr < 0x2000)
  {
    // FIXME : This RAM doesn't really exist in the NES,
    // but some homebrew roms expect it to be here.
    return vram[addr];
  }
  else if (addr < 0x3F00)
  {
    return vram[NametableMirroring(addr & 0x2FFF)];
  }
  else if (addr < 0x4000)
  {
    //Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
    u16 effective = 0x3F00 | (addr & 0x00FF);
    if (effective == 0x3F10 || effective == 0x3F14 || effective == 0x3F18 || effective == 0x3F1C)
      effective &= 0xFF0F;

    return vram[effective];
  }
  else
  {
    assert(0);
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
  u8 *pixels = frame_buffer.Data();
  if (pixel_x >= WIDTH || pixel_y >= HEIGHT)
    return;

  ////////////////////////////////////////////////////

  u8 master_palette_index_bg = ppuRead(0x3F00);

  // Get background color
  int ppu_scroll_x = scroll_x + (((PPUCTRL >> 0) & 1) ? 256 : 0);
  int ppu_scroll_y = scroll_y + (((PPUCTRL >> 1) & 1) ? 240 : 0);

  int nametable_x = (ppu_scroll_x + pixel_x) % 512;
  int nametable_y = (ppu_scroll_y + pixel_y) % 480;

  u16 which_nametable = 0;
  if (nametable_x >= 256)
  {
    which_nametable += 1;
    nametable_x -= 256;
  }

  if (nametable_y >= 240)
  {
    which_nametable += 2;
    nametable_y -= 240;
  }

  u8 bg_color_index = 0;

  // Background
  if (ShowBackground && (ShowBGInLeftMost || pixel_x >= 8))
  {
    int nametable_tile_x = nametable_x / 8;
    int nametable_tile_y = nametable_y / 8;

    u16 nametable_start = 0x2000 + 0x400 * which_nametable;
    u16 nt_byte_addr = nametable_start + 32 * nametable_tile_y + nametable_tile_x; //(nametable_tile_y * 32 + nametable_tile_x);
    u16 pattern_table_index = ppuRead(nt_byte_addr);

    u8 fine_x = nametable_x & 7;
    u8 fine_y = nametable_y & 7;

    // PPUCTRL marks whether the background tiles from from the 'left' or 'right' pattern tables.
    u16 bg_pattern_base = BGPatternTableAddress ? 0x1000 : 0x0000;

    u8 lo_bit = (ppuRead(bg_pattern_base | (pattern_table_index << 4) | 0b0000 | fine_y) >> (7 - fine_x)) & 1;
    u8 hi_bit = (ppuRead(bg_pattern_base | (pattern_table_index << 4) | 0b1000 | fine_y) >> (7 - fine_x)) & 1;
    bg_color_index = lo_bit | (hi_bit << 1);

    // Which palette (from the attribute table at the end of this nametable)
    u8 attribute_index = (nametable_tile_y / 4) * 8 + (nametable_tile_x / 4);
    u8 attribute_byte = ppuRead(nametable_start + 0x3C0 + attribute_index);
    u8 attribute_bits = ((nametable_x % 32) / 16) * 2 + ((nametable_y % 32) / 16) * 4;
    u8 bg_pal_numb = (attribute_byte >> attribute_bits) & 0b11;

    master_palette_index_bg = ppuRead(0x3F00 + 4 * bg_pal_numb + bg_color_index);
  }

  // Get sprite color
  u8 master_palette_index_sprite = 0;
  u8 sprite_color_index = 0;
  bool sprite_has_priority = false;

  if (ShowSprites && (ShowSpritesInLeftMost || pixel_x >= 8))
  {

#pragma pack(push)
#pragma pack(1)
    struct SpriteData
    {
      u8 y;
      u8 tile_index;
      u8 attributes;
      u8 x;
    };
#pragma pack(pop)

    // For each possible sprite, compute which pixel would land here

    // Are we drawing 8x16 sprites?
    const bool mode816 = (PPUCTRL & 0x20) != 0;
    const int sprite_height = mode816 ? 16 : 8;

    for (int sprite_i = 0; sprite_i < 64; ++sprite_i)
    {
      const SpriteData *sprite_data = (SpriteData *)&OAM_RAM[4 * sprite_i];
      u8 sprite_palette_num = sprite_data->attributes & 0x03;
      bool sprite_flip_horizontal = sprite_data->attributes & 0x40;
      bool sprite_flip_vertical = sprite_data->attributes & 0x80;

      if (sprite_data->x == 0)
        continue;

      int sprite_pattern_x = pixel_x - sprite_data->x;
      int sprite_pattern_y = pixel_y - (sprite_data->y + 1);

      if (sprite_pattern_x >= 0 && sprite_pattern_x < 8)
        if (sprite_pattern_y >= 0 && sprite_pattern_y < sprite_height)
        {
          u16 sprite_pattern_data_address = SpritePatternTableAddress ? 0x1000 : 0x0000;
          u16 tile_index = sprite_data->tile_index;

          int which_tile = 0;
          if (mode816)
          {
            if (sprite_pattern_y >= 8)
              which_tile = 1;

            if (sprite_flip_vertical)
              which_tile = 1 - which_tile;

            sprite_pattern_data_address = (tile_index & 1) ? 0x1000 : 0x0000;
            tile_index = (tile_index & 0xFE) + which_tile;
          }

          if (sprite_flip_horizontal)
            sprite_pattern_x = 7 - sprite_pattern_x;
          if (sprite_flip_vertical)
            sprite_pattern_y = 7 - sprite_pattern_y;

          sprite_pattern_x &= 0b111;
          sprite_pattern_y &= 0b111;

          u8 lo_bit = (ppuRead(sprite_pattern_data_address | (tile_index << 4) | 0b0000 | sprite_pattern_y) >> (7 - sprite_pattern_x)) & 1;
          u8 hi_bit = (ppuRead(sprite_pattern_data_address | (tile_index << 4) | 0b1000 | sprite_pattern_y) >> (7 - sprite_pattern_x)) & 1;
          sprite_color_index = lo_bit | (hi_bit << 1);

          master_palette_index_sprite = ppuRead(0x3F10 + 4 * sprite_palette_num + sprite_color_index);
          sprite_has_priority = (sprite_data->attributes & 0x20) == 0;

          // TODO : This is almost totally correct, but not quite
          if (ShowBackground && ShowSprites && bg_color_index && sprite_color_index && pixel_x < 255 && sprite_i == 0)
            SpriteZeroHit = 1;

          if (sprite_color_index != 0)
            break;
        }
    }
  }

  // Combine. Assume BG will 'win' and then look at cases where sprite would win instead.
  u8 output_color_index = master_palette_index_bg;
  if ((bg_color_index == 0 && sprite_color_index > 0) || (sprite_has_priority && sprite_color_index))
    output_color_index = master_palette_index_sprite;

  const auto set_pixel_color = [&](int x, int y, u8 color_index) {
    u32 pixel_num = WIDTH * y + x;

    u8 r = PALETTE_BYTES[3 * color_index + 0];
    u8 g = PALETTE_BYTES[3 * color_index + 1];
    u8 b = PALETTE_BYTES[3 * color_index + 2];

    pixels[3 * pixel_num] = r;
    pixels[3 * pixel_num + 1] = g;
    pixels[3 * pixel_num + 2] = b;
  };

  set_pixel_color(pixel_x, pixel_y, output_color_index);
}
