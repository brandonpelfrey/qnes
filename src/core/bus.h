#pragma once

#include <memory>
#include "./types.h"
#include "./cpu.h"
#include "./ppu.h"
#include "./cartridge.h"
#include "./controllers.h"

class Bus
{
private:
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<Cartridge> cartridge;
  std::shared_ptr<Controllers> controllers;

  u8 *RAM;
  u16 *RAMWriteLastPC;

public:
  Bus();
  ~Bus();

  void SetCPU(std::shared_ptr<CPU> cpu) { this->cpu = cpu; }
  void SetPPU(std::shared_ptr<PPU> ppu) { this->ppu = ppu; }
  void SetCartridge(std::shared_ptr<Cartridge> cartridge) { this->cartridge = cartridge; }
  std::shared_ptr<Cartridge> GetCartridge() { return cartridge; }

  void SetControllers(std::shared_ptr<Controllers> controllers) { this->controllers = controllers; }

  std::shared_ptr<CPU> &GetCPU() { return cpu; }

  void TriggerNMI();
  u8 Read(u16 address, bool affects_state = true);
  void Write(u16 address, u8 val);

  u8 *GetRAMView() { return RAM; }

  void SetLastRAMWritePC(u16 ram_addr, u16 pc)
  {
    RAMWriteLastPC[ram_addr & 0x7FF] = pc;
  }

  u16 GetLastRAMWritePC(u16 ram_addr)
  {
    return RAMWriteLastPC[ram_addr];
  }
};
