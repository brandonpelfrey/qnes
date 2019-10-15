#pragma once

#include <memory>
#include <string>
#include <vector>

#include "./types.h"

class Bus;
class CPUState;

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
  std::shared_ptr<Bus> bus;
  std::unique_ptr<CPUState> state;

private:
  u8 read(u16 addr);
  void write(u16 addr, u8 val);

private:
  // CPU State
  u8 a, x, y, p;
  u16 pc;
  u8 sp;

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
  u8 scratch_u8;

  enum StatusFlag
  {
    N = 1 << 7,
    V = 1 << 6,
    U = 1 << 5, // unused
    B = 1 << 4, // also unused, but has a name :)
    D = 1 << 3,
    I = 1 << 2,
    Z = 1 << 1,
    C = 1 << 0
  };

public:
  void SetFlag(StatusFlag flag, u8 val);
  u8 GetFlag(StatusFlag flag);

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

public:
  CPU();
  void Clock();
};
