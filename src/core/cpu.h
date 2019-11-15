#pragma once

#include <memory>
#include <string>
#include <vector>

#include "./cpu_debug.h"
#include "./types.h"
#include "./state.h"

class Bus;

class CPU
{
private:
  struct InstructionEntry
  {
    std::string name;
    u8 (CPU::*operation)() = nullptr;
    u8 (CPU::*addressing)() = nullptr;
    u8 cycles = 0;
  };

  std::vector<InstructionEntry> instructions;

private:
  bool m_stepmode = false;

  const u8 RWFLAGS_NO_BREAKPOINTS = 1 << 0;

  u8 read(u16 addr, u8 rw_flags = 0);
  void write(u16 addr, u8 val, u8 rw_flags = 0);
  std::shared_ptr<Bus> bus;

public:
  void InitiateOAMDMACounter();
  void SetBus(std::shared_ptr<Bus> bus) { this->bus = bus; }

private:
  // CPU State
  u8 a, x, y, p;
  u16 pc;
  u8 sp;

  enum Flags
  {
    C = 1 << 0,
    Z = 1 << 1,
    I = 1 << 2,
    D = 1 << 3,
    B = 1 << 4,
    U = 1 << 5,
    V = 1 << 6,
    N = 1 << 7,
  };

  void SetFlag(Flags flag, u8 value);
  u8 GetFlag(Flags flag);

  u16 oam_dma_cycles_remaining;
  u8 instruction_remaining_cycles;
  u32 total_clock_cycles = 0;

  // Instruction Execution Pipeline
  // 1) Addressing mode function is called
  //   - Sets addr_abs to actual address to fetch data (if any)
  //   - If relative addressing is applicable, then addr_rel is computed.
  // 2) Fetch data to 'fetched_data'
  // 3) Execute instruction logic which will act on fetched_data.
  u8 opcode;
  u16 addr_rel, addr_abs;
  u8 fetched_data;

  void SetNZ(u8 value);

private:
  // Operand addressing modes

  // These addressing mode functions correspond to the various ways that
  // data can be pulled as operands for any given instruction.

  u8 addr_implied();
  u8 addr_immediate();
  u8 addr_zeropage();
  u8 addr_zeropage_x();
  u8 addr_zeropage_y();
  u8 addr_absolute();
  u8 addr_relative();
  u8 addr_absolute_x();
  u8 addr_absolute_y();
  u8 addr_indirect();
  u8 addr_indirect_x();
  u8 addr_indirect_y();

private:
  // Op codes
  u8 ADC();
  u8 AND();
  u8 ASL();
  u8 BIT();

  u8 branchBaseInstruction(bool takeBranch);
  u8 BPL();
  u8 BMI();
  u8 BVC();
  u8 BVS();
  u8 BCC();
  u8 BCS();
  u8 BNE();
  u8 BEQ();

  u8 BRK();
  u8 CMP();
  u8 CPX();
  u8 CPY();
  u8 DEC();
  u8 EOR();

  u8 CLC();
  u8 SEC();
  u8 CLI();
  u8 SEI();
  u8 CLV();
  u8 CLD();
  u8 SED();

  u8 INC();
  u8 JMP();
  u8 JSR();
  u8 LDA();
  u8 LDX();
  u8 LDY();
  u8 LSR();

  u8 NOP();
  u8 ORA();

  u8 TAX();
  u8 TXA();
  u8 DEX();
  u8 INX();
  u8 TAY();
  u8 TYA();
  u8 DEY();
  u8 INY();

  u8 ROL();
  u8 ROR();
  u8 RTI();
  u8 RTS();
  u8 SBC();
  u8 STA();

  u8 TXS();
  u8 TSX();
  u8 PHA();
  u8 PLA();
  u8 PHP();
  u8 PLP();
  u8 STX();
  u8 STY();

private:
  u16 read16(u16 addr);
  void push(u8);
  u8 pop();

  bool in_step_mode;

public:
  CPU();
  int Step();
  void Pause();
  void Continue();
  bool IsPaused() const { return in_step_mode; }

  void TriggerNMI();
  void Clock();
  void Reset();
  void SoftReset(u16 pc);
  void Debug();
  void WTF();

  void SetPC(u16 addr) { pc = addr; }

public:
  void GetState(State *);

  struct DisassemblyEntry
  {
    u16 pc;
    u8 instruction_bytes[3];
    u8 num_instruction_bytes;

    char *buffer;
    int buffer_len;
  };
  void Disassemble(u16 addr_start, int count, DisassemblyEntry *entries);

private:
  CPUDebugging debug_state;

public:
  CPUDebugging &DebuggingControls() { return debug_state; }
};
