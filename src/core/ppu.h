#pragma once

#include <memory>
#include <functional>
#include "core/types.h"
#include "core/texture.h"

class Bus;
class Cartridge;
class PPU
{
private:
  std::shared_ptr<Bus> bus;

  u16 pixel_y;
  u16 pixel_x;

  u8 nmi_latch;
  u8 address_latch;
  u8 scroll_x;
  u8 scroll_y;

  u16 vram_addr;

  u8 PPU_DATA_read_buffer;

  union {
    u8 PPUCTRL;
    struct
    {
      u8 BaseNameTableAddress : 2;
      u8 VRAMAddressIncrement : 1;
      u8 SpritePatternTableAddress : 1;
      u8 BGPatternTableAddress : 1;
      u8 SpriteSize : 1;
      u8 PPUMasterSlace : 1;
      u8 GenerateNMIOnVBI : 1;
    };
  };

  union {
    u8 PPUMASK;
    struct
    {
      u8 Greyscale : 1;
      u8 ShowBGInLeftMost : 1;
      u8 ShowSpritesInLeftMost : 1;
      u8 ShowBackground : 1;
      u8 ShowSprites : 1;
      u8 EmphasizeRed : 1;
      u8 EmphasizeGreen : 1;
      u8 EmphasizeBlue : 1;
    };
  };

  union {
    u8 PPUSTATUS;
#pragma pack(push)
#pragma pack(1)
    struct
    {
      u8 LeastSignificantBitsForRecentWrite : 5;
      u8 SpriteOverflow : 1;
      u8 SpriteZeroHit : 1;
      u8 VerticalBlank : 1;
    };
#pragma pack(pop)
  };

  u8 OAMADDR;

  // 16KB address space, some of it is mirrors.
  u8 *vram;
  u8 *OAM_RAM;

  Texture frame_buffer;
  Texture pattern_left;
  Texture pattern_right;
  std::shared_ptr<Cartridge> cart;

  void render_pixel();
  void render_pattern_tables();

  u16 NametableMirroring(u16 addr);
  u8 ppuRead(u16 addr);

public:
  PPU();
  ~PPU();

  void SetBus(std::shared_ptr<Bus> bus) { this->bus = bus; }
  void SetCartridge(std::shared_ptr<Cartridge> &cart) { this->cart = cart; }

  u8 Read(u16 addr);
  void Write(u16 addr, u8 val);

  // Advance by one clock cycle (1/3 of a CPU cycle, 1 pixel)
  void Clock();

  Texture &GetFrameBufferTexture() { return frame_buffer; }
  Texture &GetPatternTableLeftTexture() { return pattern_left; }

  std::function<void()> endFrameCallBack;
  void SetEndFrameCallBack(std::function<void()> endFrameCallBack)
  {
    this->endFrameCallBack = endFrameCallBack;
  }
};
