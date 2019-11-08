#include <functional>
#include <string>

#include "core/cpu.h"
#include "core/bus.h"
#include "core/state.h"

// https://www.masswerk.at/6502/6502_instruction_set.html
// http://www.6502.org/tutorials/6502opcodes.html#BRK

// Cycle accounting done here is taken from the ideas of olcNES
// by "OneLineCoder". It allows for interleaving operations
// from multiple pieces of hardware when some long-running
// instructions are in flight. This should be close to cycle-accurate
// as a result.

CPU::CPU()
{
  instructions.resize(256);

  // ADC
  instructions[0x69] = {"ADC", &CPU::ADC, &CPU::addr_immediate, 2};
  instructions[0x65] = {"ADC", &CPU::ADC, &CPU::addr_zeropage, 3};
  instructions[0x75] = {"ADC", &CPU::ADC, &CPU::addr_zeropage_x, 4};
  instructions[0x6D] = {"ADC", &CPU::ADC, &CPU::addr_absolute, 4};
  instructions[0x7D] = {"ADC", &CPU::ADC, &CPU::addr_absolute_x, 4};
  instructions[0x79] = {"ADC", &CPU::ADC, &CPU::addr_absolute_y, 4};
  instructions[0x61] = {"ADC", &CPU::ADC, &CPU::addr_indirect_x, 6};
  instructions[0x71] = {"ADC", &CPU::ADC, &CPU::addr_indirect_y, 5};

  // AND
  instructions[0x29] = {"AND", &CPU::AND, &CPU::addr_immediate, 2};
  instructions[0x25] = {"AND", &CPU::AND, &CPU::addr_zeropage, 3};
  instructions[0x35] = {"AND", &CPU::AND, &CPU::addr_zeropage_x, 4};
  instructions[0x2D] = {"AND", &CPU::AND, &CPU::addr_absolute, 4};
  instructions[0x3D] = {"AND", &CPU::AND, &CPU::addr_absolute_x, 4};
  instructions[0x39] = {"AND", &CPU::AND, &CPU::addr_absolute_y, 4};
  instructions[0x21] = {"AND", &CPU::AND, &CPU::addr_indirect_x, 6};
  instructions[0x31] = {"AND", &CPU::AND, &CPU::addr_indirect_y, 5};

  // ASL
  instructions[0x0A] = {"ASL", &CPU::ASL, &CPU::addr_implied, 2};
  instructions[0x06] = {"ASL", &CPU::ASL, &CPU::addr_zeropage, 5};
  instructions[0x16] = {"ASL", &CPU::ASL, &CPU::addr_zeropage_x, 6};
  instructions[0x0E] = {"ASL", &CPU::ASL, &CPU::addr_absolute, 6};
  instructions[0x1E] = {"ASL", &CPU::ASL, &CPU::addr_absolute_x, 7};

  // BIT
  instructions[0x24] = {"BIT", &CPU::BIT, &CPU::addr_zeropage, 3};
  instructions[0x2C] = {"BIT", &CPU::BIT, &CPU::addr_absolute, 4};

  // Branch instructions
  instructions[0x10] = {"BPL", &CPU::BPL, &CPU::addr_relative, 2};
  instructions[0x30] = {"BMI", &CPU::BMI, &CPU::addr_relative, 2};
  instructions[0x50] = {"BVC", &CPU::BVC, &CPU::addr_relative, 2};
  instructions[0x70] = {"BVS", &CPU::BVS, &CPU::addr_relative, 2};
  instructions[0x90] = {"BCC", &CPU::BCC, &CPU::addr_relative, 2};
  instructions[0xB0] = {"BCS", &CPU::BCS, &CPU::addr_relative, 2};
  instructions[0xD0] = {"BNE", &CPU::BNE, &CPU::addr_relative, 2};
  instructions[0xF0] = {"BEQ", &CPU::BEQ, &CPU::addr_relative, 2};

  // BRK
  instructions[0x00] = {"BRK", &CPU::BRK, &CPU::addr_implied, 7};

  // CMP
  instructions[0xC9] = {"CMP", &CPU::CMP, &CPU::addr_immediate, 2};
  instructions[0xC5] = {"CMP", &CPU::CMP, &CPU::addr_zeropage, 3};
  instructions[0xD5] = {"CMP", &CPU::CMP, &CPU::addr_zeropage_x, 4};
  instructions[0xCD] = {"CMP", &CPU::CMP, &CPU::addr_absolute, 4};
  instructions[0xDD] = {"CMP", &CPU::CMP, &CPU::addr_absolute_x, 4};
  instructions[0xD9] = {"CMP", &CPU::CMP, &CPU::addr_absolute_y, 4};
  instructions[0xC1] = {"CMP", &CPU::CMP, &CPU::addr_indirect_x, 6};
  instructions[0xD1] = {"CMP", &CPU::CMP, &CPU::addr_indirect_y, 5};

  // CPX
  instructions[0xE0] = {"CPX", &CPU::CPX, &CPU::addr_immediate, 2};
  instructions[0xE4] = {"CPX", &CPU::CPX, &CPU::addr_zeropage, 3};
  instructions[0xEC] = {"CPX", &CPU::CPX, &CPU::addr_absolute, 4};

  // CPY
  instructions[0xC0] = {"CPY", &CPU::CPY, &CPU::addr_immediate, 2};
  instructions[0xC4] = {"CPY", &CPU::CPY, &CPU::addr_zeropage, 3};
  instructions[0xCC] = {"CPY", &CPU::CPY, &CPU::addr_absolute, 4};

  // DEC
  instructions[0xC6] = {"DEC", &CPU::DEC, &CPU::addr_zeropage, 5};
  instructions[0xD6] = {"DEC", &CPU::DEC, &CPU::addr_zeropage_x, 6};
  instructions[0xCE] = {"DEC", &CPU::DEC, &CPU::addr_absolute, 6};
  instructions[0xDE] = {"DEC", &CPU::DEC, &CPU::addr_absolute_x, 7};

  // EOR
  instructions[0x49] = {"EOR", &CPU::EOR, &CPU::addr_immediate, 2};
  instructions[0x45] = {"EOR", &CPU::EOR, &CPU::addr_zeropage, 3};
  instructions[0x55] = {"EOR", &CPU::EOR, &CPU::addr_zeropage_x, 4};
  instructions[0x4D] = {"EOR", &CPU::EOR, &CPU::addr_absolute, 4};
  instructions[0x5D] = {"EOR", &CPU::EOR, &CPU::addr_absolute_x, 4};
  instructions[0x59] = {"EOR", &CPU::EOR, &CPU::addr_absolute_y, 5};
  instructions[0x41] = {"EOR", &CPU::EOR, &CPU::addr_indirect_x, 6};
  instructions[0x51] = {"EOR", &CPU::EOR, &CPU::addr_indirect_y, 5};

  // Flag instructions
  instructions[0x18] = {"CLC", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0x38] = {"SEC", &CPU::SEC, &CPU::addr_implied, 2};
  instructions[0x58] = {"CLI", &CPU::CLI, &CPU::addr_implied, 2};
  instructions[0x78] = {"SEI", &CPU::SEI, &CPU::addr_implied, 2};
  instructions[0xB8] = {"CLV", &CPU::CLV, &CPU::addr_implied, 2};
  instructions[0xD8] = {"CLD", &CPU::CLD, &CPU::addr_implied, 2};
  instructions[0xF8] = {"SED", &CPU::SED, &CPU::addr_implied, 2};

  // INC
  instructions[0xE6] = {"INC", &CPU::INC, &CPU::addr_zeropage, 5};
  instructions[0xF6] = {"INC", &CPU::INC, &CPU::addr_zeropage_x, 6};
  instructions[0xEE] = {"INC", &CPU::INC, &CPU::addr_absolute, 6};
  instructions[0xFE] = {"INC", &CPU::INC, &CPU::addr_absolute_x, 7};

  // JMP
  instructions[0x4C] = {"JMP", &CPU::JMP, &CPU::addr_absolute, 3};
  instructions[0x6C] = {"JMP", &CPU::JMP, &CPU::addr_indirect, 5};

  // JSR
  instructions[0x20] = {"JSR", &CPU::JSR, &CPU::addr_absolute, 6};

  // LDA
  instructions[0xA9] = {"LDA", &CPU::LDA, &CPU::addr_immediate, 2};
  instructions[0xA5] = {"LDA", &CPU::LDA, &CPU::addr_zeropage, 3};
  instructions[0xB5] = {"LDA", &CPU::LDA, &CPU::addr_zeropage_x, 4};
  instructions[0xAD] = {"LDA", &CPU::LDA, &CPU::addr_absolute, 4};
  instructions[0xBD] = {"LDA", &CPU::LDA, &CPU::addr_absolute_x, 4};
  instructions[0xB9] = {"LDA", &CPU::LDA, &CPU::addr_absolute_y, 4};
  instructions[0xA1] = {"LDA", &CPU::LDA, &CPU::addr_indirect_x, 6};
  instructions[0xB1] = {"LDA", &CPU::LDA, &CPU::addr_indirect_y, 5};

  // LDX
  instructions[0xA2] = {"LDX", &CPU::LDX, &CPU::addr_immediate, 2};
  instructions[0xA6] = {"LDX", &CPU::LDX, &CPU::addr_zeropage, 3};
  instructions[0xB6] = {"LDX", &CPU::LDX, &CPU::addr_zeropage_y, 4};
  instructions[0xAE] = {"LDX", &CPU::LDX, &CPU::addr_absolute, 4};
  instructions[0xBE] = {"LDX", &CPU::LDX, &CPU::addr_absolute_y, 4};

  // LDY
  instructions[0xA0] = {"LDY", &CPU::LDY, &CPU::addr_immediate, 2};
  instructions[0xA4] = {"LDY", &CPU::LDY, &CPU::addr_zeropage, 3};
  instructions[0xB4] = {"LDY", &CPU::LDY, &CPU::addr_zeropage_x, 4};
  instructions[0xAC] = {"LDY", &CPU::LDY, &CPU::addr_absolute, 4};
  instructions[0xBC] = {"LDY", &CPU::LDY, &CPU::addr_absolute_x, 4};

  // LSR
  instructions[0x4A] = {"LSR", &CPU::LSR, &CPU::addr_implied, 2};
  instructions[0x46] = {"LSR", &CPU::LSR, &CPU::addr_zeropage, 5};
  instructions[0x56] = {"LSR", &CPU::LSR, &CPU::addr_zeropage_x, 6};
  instructions[0x4E] = {"LSR", &CPU::LSR, &CPU::addr_absolute, 6};
  instructions[0x5E] = {"LSR", &CPU::LSR, &CPU::addr_absolute_x, 7};

  // NOP
  instructions[0xDA] = {"NOP", &CPU::NOP, &CPU::addr_implied, 2};
  instructions[0xEA] = {"NOP", &CPU::NOP, &CPU::addr_implied, 2};
  instructions[0xFA] = {"NOP", &CPU::NOP, &CPU::addr_implied, 2};

  // ORA
  instructions[0x09] = {"ORA", &CPU::ORA, &CPU::addr_immediate, 2};
  instructions[0x05] = {"ORA", &CPU::ORA, &CPU::addr_zeropage, 3};
  instructions[0x15] = {"ORA", &CPU::ORA, &CPU::addr_zeropage_x, 4};
  instructions[0x0D] = {"ORA", &CPU::ORA, &CPU::addr_absolute, 4};
  instructions[0x1D] = {"ORA", &CPU::ORA, &CPU::addr_absolute_x, 4};
  instructions[0x19] = {"ORA", &CPU::ORA, &CPU::addr_absolute_y, 4};
  instructions[0x01] = {"ORA", &CPU::ORA, &CPU::addr_indirect_x, 6};
  instructions[0x11] = {"ORA", &CPU::ORA, &CPU::addr_indirect_y, 5};

  // Register instructions
  instructions[0xAA] = {"TAX", &CPU::TAX, &CPU::addr_implied, 2};
  instructions[0x8A] = {"TXA", &CPU::TXA, &CPU::addr_implied, 2};
  instructions[0xCA] = {"DEX", &CPU::DEX, &CPU::addr_implied, 2};
  instructions[0xE8] = {"INX", &CPU::INX, &CPU::addr_implied, 2};
  instructions[0xA8] = {"TAY", &CPU::TAY, &CPU::addr_implied, 2};
  instructions[0x98] = {"TYA", &CPU::TYA, &CPU::addr_implied, 2};
  instructions[0x88] = {"DEY", &CPU::DEY, &CPU::addr_implied, 2};
  instructions[0xC8] = {"INY", &CPU::INY, &CPU::addr_implied, 2};

  // ROL
  instructions[0x2A] = {"ROL", &CPU::ROL, &CPU::addr_implied, 2};
  instructions[0x26] = {"ROL", &CPU::ROL, &CPU::addr_zeropage, 5};
  instructions[0x36] = {"ROL", &CPU::ROL, &CPU::addr_zeropage_x, 6};
  instructions[0x2E] = {"ROL", &CPU::ROL, &CPU::addr_absolute, 6};
  instructions[0x3E] = {"ROL", &CPU::ROL, &CPU::addr_absolute_x, 7};

  // ROR
  instructions[0x6A] = {"ROR", &CPU::ROR, &CPU::addr_implied, 2};
  instructions[0x66] = {"ROR", &CPU::ROR, &CPU::addr_zeropage, 5};
  instructions[0x76] = {"ROR", &CPU::ROR, &CPU::addr_zeropage_x, 6};
  instructions[0x6E] = {"ROR", &CPU::ROR, &CPU::addr_absolute, 6};
  instructions[0x7E] = {"ROR", &CPU::ROR, &CPU::addr_absolute_x, 7};

  // RTI
  instructions[0x40] = {"RTI", &CPU::RTI, &CPU::addr_implied, 6};

  // RTS
  instructions[0x60] = {"RTS", &CPU::RTS, &CPU::addr_implied, 6};

  // SBC
  instructions[0xE9] = {"SBC", &CPU::SBC, &CPU::addr_immediate, 2};
  instructions[0xE5] = {"SBC", &CPU::SBC, &CPU::addr_zeropage, 3};
  instructions[0xF5] = {"SBC", &CPU::SBC, &CPU::addr_zeropage_x, 4};
  instructions[0xED] = {"SBC", &CPU::SBC, &CPU::addr_absolute, 4};
  instructions[0xFD] = {"SBC", &CPU::SBC, &CPU::addr_absolute_x, 4};
  instructions[0xF9] = {"SBC", &CPU::SBC, &CPU::addr_absolute_y, 4};
  instructions[0xE1] = {"SBC", &CPU::SBC, &CPU::addr_indirect_x, 6};
  instructions[0xF1] = {"SBC", &CPU::SBC, &CPU::addr_indirect_y, 5};

  // STA
  instructions[0x85] = {"STA", &CPU::STA, &CPU::addr_zeropage, 3};
  instructions[0x95] = {"STA", &CPU::STA, &CPU::addr_zeropage_x, 4};
  instructions[0x8D] = {"STA", &CPU::STA, &CPU::addr_absolute, 4};
  instructions[0x9D] = {"STA", &CPU::STA, &CPU::addr_absolute_x, 5};
  instructions[0x99] = {"STA", &CPU::STA, &CPU::addr_absolute_y, 5};
  instructions[0x81] = {"STA", &CPU::STA, &CPU::addr_indirect_x, 6};
  instructions[0x91] = {"STA", &CPU::STA, &CPU::addr_indirect_y, 6};

  // Stack instructions
  instructions[0x9A] = {"TXS", &CPU::TXS, &CPU::addr_implied, 2};
  instructions[0xBA] = {"TSX", &CPU::TSX, &CPU::addr_implied, 2};
  instructions[0x48] = {"PHA", &CPU::PHA, &CPU::addr_implied, 3};
  instructions[0x68] = {"PLA", &CPU::PLA, &CPU::addr_implied, 4};
  instructions[0x08] = {"PHP", &CPU::PHP, &CPU::addr_implied, 3};
  instructions[0x28] = {"PLP", &CPU::PLP, &CPU::addr_implied, 4};

  // STX
  instructions[0x86] = {"STX", &CPU::STX, &CPU::addr_zeropage, 3};
  instructions[0x96] = {"STX", &CPU::STX, &CPU::addr_zeropage_y, 4};
  instructions[0x8E] = {"STX", &CPU::STX, &CPU::addr_absolute, 4};

  // STY
  instructions[0x84] = {"STY", &CPU::STY, &CPU::addr_zeropage, 3};
  instructions[0x94] = {"STY", &CPU::STY, &CPU::addr_zeropage_x, 4};
  instructions[0x8C] = {"STY", &CPU::STY, &CPU::addr_absolute, 4};

  in_step_mode = false;
}

void CPU::Pause()
{
  in_step_mode = true;
}

void CPU::Continue()
{
  in_step_mode = false;
}

////////////////////////////////////////////////////////////////////////////////////////
// CPU Utilities

void CPU::Reset()
{
  p = 0x24;
  a = x = y = 0;
  sp = 0xFD;

  pc = read(0xFFFC) | (read(0xFFFD) << 8);
}

void CPU::SoftReset(u16 newpc)
{
  Reset();
  pc = newpc;
}

////////////////////////////////////////////////////////////////////////////////////////
// Instruction Implementations

void CPU::SetFlag(Flags flag, u8 value)
{
  if (value != 0)
    p |= flag;
  else
    p &= (~flag) & 0xFF;
}

u8 CPU::GetFlag(Flags flag)
{
  return p & flag ? 1 : 0;
}

// This is such a common thing in instructions, that we'll just wrap this into one function.
void CPU::SetNZ(u8 value)
{
  SetFlag(N, value & 0x80);
  SetFlag(Z, value == 0);
}

u8 CPU::ADC()
{
  u8 arg = read(addr_abs);
  u16 result = arg + GetFlag(C) + a;

  // If both inputs had the same sign but the result has a different sign, then set V.
  bool overflowCheck = (a & 0x80) == (arg & 0x80) && (a & 0x80) != (result & 0x80);

  a = result & 0xFF;
  SetNZ(a);
  SetFlag(C, result > 0xFF);
  SetFlag(V, overflowCheck);

  return 0;
}

u8 CPU::AND()
{
  a &= read(addr_abs);
  SetNZ(a);
  return 0;
}

u8 CPU::ASL()
{
  u8 fetched = read(addr_abs);
  SetFlag(C, fetched & 0x80);
  u8 temp = (fetched << 1) & 0xFF;
  SetNZ(temp);

  if (instructions[opcode].addressing == &CPU::addr_implied)
    a = temp;
  else
    write(addr_abs, temp);
  return 0;
}

u8 CPU::BIT()
{
  u8 fetched = read(addr_abs);
  u8 temp = a & fetched;

  SetFlag(Z, temp == 0);
  SetFlag(N, fetched & (1 << 7));
  SetFlag(V, fetched & (1 << 6));
  return 0;
}

u8 CPU::branchBaseInstruction(bool takeBranch)
{
  //printf("Take Branch ? == %u\n", takeBranch);
  if (takeBranch)
  {
    // If we take the branch, there's an extra cycle.
    instruction_remaining_cycles++;

    // Compute the actual branch location from the relative offset.
    //printf("addr_abs = pc + addr_rel ----- 0x%04X = 0x%04X + 0x%04X\n", addr_abs, pc, addr_rel);
    addr_abs = pc + addr_rel;

    // If the new address crosses a page boundary, then another.
    if ((addr_abs & 0xFF00) != (pc & 0xFF00))
      instruction_remaining_cycles++;

    pc = addr_abs;
  }
  return 0;
}

// Branch on 'plus' (positive)
u8 CPU::BPL() { return branchBaseInstruction(GetFlag(N) == 0); }

// Branch on 'minus' (negative)
u8 CPU::BMI() { return branchBaseInstruction(GetFlag(N) == 1); }

// Branch on overflow clear
u8 CPU::BVC() { return branchBaseInstruction(GetFlag(V) == 0); }

// Branch on overflow set
u8 CPU::BVS() { return branchBaseInstruction(GetFlag(V) == 1); }

// Branch on carry clear
u8 CPU::BCC() { return branchBaseInstruction(GetFlag(C) == 0); }

// Branch on carry set
u8 CPU::BCS() { return branchBaseInstruction(GetFlag(C) == 1); }

// Branch on not-equal
u8 CPU::BNE() { return branchBaseInstruction(GetFlag(Z) == 0); }

// Branch on equal
u8 CPU::BEQ() { return branchBaseInstruction(GetFlag(Z) == 1); }

u8 CPU::BRK()
{
  pc++;

  SetFlag(I, 1);
  push(pc >> 8);
  push(pc & 0xFF);

  SetFlag(B, 1);
  push(p);
  SetFlag(B, 0);

  pc = read(0xFFFE) | (read(0xFFFF) << 8);
  return 0;
}

u8 CPU::CMP()
{
  u8 arg = read(addr_abs);
  u8 result = a - arg;
  SetNZ(result);
  SetFlag(C, a >= arg);
  return 0;
}

u8 CPU::CPX()
{
  u8 arg = read(addr_abs);
  u8 result = x - arg;
  SetNZ(result);
  SetFlag(C, x >= arg);
  return 0;
}

u8 CPU::CPY()
{
  u8 arg = read(addr_abs);
  u8 result = y - arg;
  SetNZ(result);
  SetFlag(C, y >= arg);
  return 0;
}

u8 CPU::DEC()
{
  u8 temp = read(addr_abs);
  temp--;
  SetNZ(temp);
  write(addr_abs, temp);
  return 0;
}

u8 CPU::EOR()
{
  a ^= read(addr_abs);
  SetNZ(a);
  return 0;
}

u8 CPU::CLC()
{
  SetFlag(C, 0);
  return 0;
}
u8 CPU::SEC()
{
  SetFlag(C, 1);
  return 0;
}
u8 CPU::CLI()
{
  SetFlag(I, 0);
  return 0;
}
u8 CPU::SEI()
{
  SetFlag(I, 1);
  return 0;
}
u8 CPU::CLV()
{
  SetFlag(V, 0);
  return 0;
}
u8 CPU::CLD()
{
  SetFlag(D, 0);
  return 0;
}

u8 CPU::SED()
{
  SetFlag(D, 1);
  return 0;
}

u8 CPU::INC()
{
  u8 temp = read(addr_abs);
  temp++;
  write(addr_abs, temp);
  SetNZ(temp);
  return 0;
}

u8 CPU::JMP()
{
  pc = addr_abs;
  return 0;
}

u8 CPU::JSR()
{
  pc--;

  push(pc >> 8);
  push(pc & 0xFF);

  pc = addr_abs;
  return 0;
}

u8 CPU::LDA()
{
  a = read(addr_abs);
  SetNZ(a);
  return 0;
}

u8 CPU::LDX()
{
  x = read(addr_abs);
  SetNZ(x);
  return 0;
}

u8 CPU::LDY()
{
  y = read(addr_abs);
  SetNZ(y);
  return 0;
}

u8 CPU::LSR()
{
  u8 val;
  if (instructions[opcode].addressing == &CPU::addr_implied)
    val = a;
  else
    val = read(addr_abs);

  SetFlag(C, val & 1);
  val = val >> 1;
  SetNZ(val);

  if (instructions[opcode].addressing == &CPU::addr_implied)
    a = val;
  else
    write(addr_abs, val);

  return 0;
}

u8 CPU::NOP()
{
  return 0;
}

u8 CPU::ORA()
{
  a |= read(addr_abs);
  SetNZ(a);
  return 0;
}

u8 CPU::TAX()
{
  x = a;
  SetNZ(x);
  return 0;
}
u8 CPU::TXA()
{
  a = x;
  SetNZ(a);
  return 0;
}
u8 CPU::DEX()
{
  x--;
  SetNZ(x);
  return 0;
}
u8 CPU::INX()
{
  x++;
  SetNZ(x);
  return 0;
}
u8 CPU::TAY()
{
  y = a;
  SetNZ(y);
  return 0;
}
u8 CPU::TYA()
{
  a = y;
  SetNZ(a);
  return 0;
}
u8 CPU::DEY()
{
  y--;
  SetNZ(y);
  return 0;
}
u8 CPU::INY()
{
  y++;
  SetNZ(y);
  return 0;
}

u8 CPU::ROL()
{
  u8 val;
  if (instructions[opcode].addressing == &CPU::addr_implied)
    val = a;
  else
    val = read(addr_abs);

  u8 newC = (val & 0x80) ? 1 : 0;
  val = (val << 1) | GetFlag(C);
  SetNZ(val);
  SetFlag(C, newC);

  if (instructions[opcode].addressing == &CPU::addr_implied)
    a = val;
  else
    write(addr_abs, val);
  return 0;
}

u8 CPU::ROR()
{
  u8 val;
  if (instructions[opcode].addressing == &CPU::addr_implied)
    val = a;
  else
    val = read(addr_abs);

  u8 newC = val & 1;
  val = (val >> 1) | (GetFlag(C) << 7);

  SetNZ(val);
  SetFlag(C, newC);

  if (instructions[opcode].addressing == &CPU::addr_implied)
    a = val;
  else
    write(addr_abs, val);
  return 0;
}

u8 CPU::RTI()
{
  p = pop();
  SetFlag(B, 0);
  SetFlag(U, 0);

  u16 low = pop() & 0xFF;
  u16 high = pop() & 0xFF;
  pc = low | (high << 8);
  return 0;
}

u8 CPU::RTS()
{
  u16 low = pop() & 0xFF;
  u16 high = pop() & 0xFF;
  pc = low | (high << 8);
  pc++;
  return 0;
}

u8 CPU::SBC()
{
  u8 arg = read(addr_abs) ^ 0xFF;
  u16 result = arg + GetFlag(C) + a;

  // If both inputs had the same sign but the result has a different sign, then set V.
  bool overflowCheck = (result ^ a) & (result ^ arg) & 0x0080; //(a & 0x80) == (arg & 0x80) && (a & 0x80) != (result & 0x80);

  a = result & 0xFF;
  SetNZ(a);
  SetFlag(C, result > 0xFF);
  SetFlag(V, overflowCheck);

  return 0;
}

u8 CPU::STA()
{
  write(addr_abs, a);
  return 0;
}

u8 CPU::TXS()
{
  sp = x;
  return 0;
}

u8 CPU::TSX()
{
  x = sp;
  SetNZ(x);
  return 0;
}

// Push accumulator
u8 CPU::PHA()
{
  push(a);
  return 0;
}

u8 CPU::PLA()
{
  a = pop();
  SetNZ(a);
  return 0;
}

u8 CPU::PHP()
{
  SetFlag(B, 1);
  SetFlag(U, 1);
  push(p);
  SetFlag(B, 0);
  SetFlag(U, 0);
  return 0;
}

u8 CPU::PLP()
{
  p = pop();
  SetFlag(U, 1);
  return 0;
}

u8 CPU::STX()
{
  write(addr_abs, x);
  return 0;
}

u8 CPU::STY()
{
  write(addr_abs, y);
  return 0;
}

///////////////////////////////////////////////////////////////
// Addressing Modes

u8 CPU::addr_implied()
{
  return 0;
}

u8 CPU::addr_immediate()
{
  addr_abs = pc;
  pc++;
  return 0;
}

u8 CPU::addr_zeropage()
{
  addr_abs = read(pc) & 0x00FF;
  pc++;
  return 0;
}

u8 CPU::addr_zeropage_x()
{
  addr_abs = (read(pc) + x) & 0x00FF;
  pc++;
  return 0;
}

u8 CPU::addr_zeropage_y()
{
  addr_abs = (read(pc) + y) & 0x00FF;
  pc++;
  return 0;
}

u8 CPU::addr_absolute()
{
  // low byte first, then high byte (6502 is little endian)
  u8 low = read(pc);
  u8 high = read(pc + 1);
  pc += 2;

  addr_abs = (high << 8) | low;
  return 0;
}

u16 sign_extend_16(u8 val)
{
  if (val & 0x80)
    return 0xFF00 | val;
  return val;
}

// Only the branch instructions use this.
u8 CPU::addr_relative()
{
  addr_rel = read(pc);
  pc++;
  addr_rel = sign_extend_16(addr_rel);
  return 0;
}

u8 CPU::addr_absolute_x()
{
  // Just like absolute, but need to check for page crossing
  u8 low = read(pc++);
  u8 high = read(pc++);
  addr_abs = (high << 8) | low;
  addr_abs += x;

  bool crossed_page = (addr_abs & 0xFF00) != (high << 8);
  return crossed_page ? 1 : 0;
}

u8 CPU::addr_absolute_y()
{
  // Just like absolute, but need to check for page crossing
  u8 low = read(pc++);
  u8 high = read(pc++);
  addr_abs = (high << 8) | low;
  addr_abs += y;

  bool crossed_page = (addr_abs & 0xFF00) != (high << 8);
  return crossed_page ? 1 : 0;
}

u8 CPU::addr_indirect()
{
  u8 low = read(pc++);
  u8 high = read(pc++);
  u16 ptr = (high << 8) | low;

  if (low == 0x00FF) // Simulate page boundary hardware bug
  {
    addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr + 0);
  }
  else // Behave normally
  {
    addr_abs = (read(ptr + 1) << 8) | read(ptr + 0);
  }

  return 0;
}

u8 CPU::addr_indirect_x()
{
  u16 ptr_addr = read(pc++) & 0xFF;

  u8 low = read((ptr_addr + x) & 0xFF);
  u8 high = read((ptr_addr + x + 1) & 0xFF);
  addr_abs = (high << 8) | low;
  return 0;
}

u8 CPU::addr_indirect_y()
{
  u16 t = read(pc++) & 0xFF;
  u16 low = read(t & 0xFF);
  u16 high = read((t + 1) & 0xFF);

  addr_abs = (high << 8) | low;
  addr_abs += y;

  return (addr_abs >> 8) != high ? 1 : 0;
}

u8 CPU::read(u16 addr, u8 rw_flags)
{
  if (rw_flags & RWFLAGS_NO_BREAKPOINTS)
    return bus->Read(addr);
  else
  {
    m_stepmode = true;
    return bus->Read(addr);
  }
}

void CPU::write(u16 addr, u8 val, u8 rw_flags)
{
  bus->Write(addr, val);
}

void CPU::Clock()
{
  // While OAM DMA is taking place, the CPU doesn't do anything else.
  if (oam_dma_cycles_remaining > 0)
  {
    oam_dma_cycles_remaining--;
    total_clock_cycles++;
    return;
  }

  if (instruction_remaining_cycles == 0)
  {
    // Read the next instruction
    opcode = read(pc++);

    u8 operation_base_cycles = instructions[opcode].cycles;

    if (instructions[opcode].addressing != nullptr)
    {
      u8 address_mode_cycles = (this->*instructions[opcode].addressing)();
      u8 operation_extra_cycles = (this->*instructions[opcode].operation)();

      instruction_remaining_cycles = operation_base_cycles + address_mode_cycles + operation_extra_cycles;
    }
    else
    {

      instruction_remaining_cycles = 1;
    }
  }

  instruction_remaining_cycles--;
  total_clock_cycles++;
}

bool WTF_MODE = true;
const int WTF_LEN = 128;
int WTF_PTR = 0;
char WTF_BUFFER[1024][WTF_LEN];

void CPU::WTF()
{
  for (int i = 0; i < WTF_LEN; ++i)
  {
    printf("WTF[%d] -- %s", 1 + i - WTF_LEN, WTF_BUFFER[(WTF_PTR + i) % WTF_LEN]);
  }
  exit(0);
}

int CPU::Step()
{
  //Debug();

  int total_step_cycles = 0;

  // Finish any remaining instruction which was already in flight
  while (instruction_remaining_cycles > 0)
  {
    Clock();
    total_step_cycles++;
  }

  // Start the next instruction, then run it to completion
  Clock();
  total_step_cycles++;

  while (instruction_remaining_cycles > 0)
  {
    Clock();
    total_step_cycles++;
  }

  return total_step_cycles;
}

void CPU::TriggerNMI()
{
  push((pc >> 8) & 0xFF);
  push(pc & 0xFF);

  SetFlag(B, 0);
  SetFlag(U, 1);
  SetFlag(I, 1);
  push(p);

  pc = read16(0xFFFA);
  printf("NMI @ 0x%04X\n", pc);
}

u16 CPU::read16(u16 addr)
{
  u16 low = read(addr) & 0xFF;
  u16 high = read(addr + 1) & 0xFF;
  return (high << 8) | low;
}

void CPU::push(u8 val)
{
  bus->Write(0x100 | sp, val);
  sp--;
}

u8 CPU::pop()
{
  sp++;
  return bus->Read(0x100 | sp);
}

void CPU::InitiateOAMDMACounter()
{
  oam_dma_cycles_remaining = 513;
}

void CPU::Debug()
{
  char disassembly_string[256];
  char *disassembly = disassembly_string;

  disassembly += sprintf(disassembly, "0x%04X ", pc);

  u8 opcode = read(pc);
  auto entry = instructions[opcode];

  u8 pc1 = read(pc + 1);
  u8 pc2 = read(pc + 2);
  u16 branch_address = pc + 2 + sign_extend_16(pc1);

  if (entry.addressing == &CPU::addr_implied)
  {
    disassembly += sprintf(disassembly, "%s", entry.name.c_str());
  }
  else if (entry.addressing == &CPU::addr_immediate)
  {
    disassembly += sprintf(disassembly, "%s #$%02X", entry.name.c_str(), pc1);
  }
  else if (entry.addressing == &CPU::addr_absolute)
  {
    disassembly += sprintf(disassembly, "%s $%02X%02X", entry.name.c_str(), pc2, pc1);
  }
  else if (entry.addressing == &CPU::addr_absolute_x)
  {
    disassembly += sprintf(disassembly, "%s $%02X%02X,X", entry.name.c_str(), pc2, pc1);
  }
  else if (entry.addressing == &CPU::addr_absolute_y)
  {
    disassembly += sprintf(disassembly, "%s $%02X%02X,Y", entry.name.c_str(), pc2, pc1);
  }
  else if (entry.addressing == &CPU::addr_zeropage)
  {
    disassembly += sprintf(disassembly, "%s $%02X", entry.name.c_str(), pc1);
  }
  else if (entry.addressing == &CPU::addr_zeropage_x)
  {
    disassembly += sprintf(disassembly, "%s $%02X,X", entry.name.c_str(), pc1);
  }
  else if (entry.addressing == &CPU::addr_zeropage_y)
  {
    disassembly += sprintf(disassembly, "%s $%02X,Y", entry.name.c_str(), pc1);
  }
  else if (entry.addressing == &CPU::addr_indirect)
  {
    disassembly += sprintf(disassembly, "%s ($%02X%02X)", entry.name.c_str(), pc2, pc1);
    // TODO : show indirect pointer
  }
  else if (entry.addressing == &CPU::addr_indirect_x)
  {
    disassembly += sprintf(disassembly, "%s ($%02X,X)", entry.name.c_str(), pc1);
    // TODO : show indirect pointer
  }
  else if (entry.addressing == &CPU::addr_indirect_y)
  {
    disassembly += sprintf(disassembly, "%s ($%02X),Y", entry.name.c_str(), pc1);
    // TODO : show indirect pointer
  }
  else if (entry.addressing == &CPU::addr_relative)
  {
    disassembly += sprintf(disassembly, "%s $%04X", entry.name.c_str(), branch_address);
  }
  else
  {
    disassembly += sprintf(disassembly, "??? opcode 0x%02X", opcode);
  }

  static char line_buffer[1024];
  char *line_buffer_current = line_buffer;

  line_buffer_current += sprintf(line_buffer_current, "%-20s ", disassembly_string);
  line_buffer_current += sprintf(line_buffer_current, ":: %010u ::  A:%02X X:%02X Y:%02X P:%02X SP:%02X  --  ", total_clock_cycles, a, x, y, p, sp);

  const int W = 3;
  for (int i = -W; i <= W; ++i)
  {
    u16 q = 0x100 | ((sp + i) & 0xFF);

    if (i == 0)
    {
      line_buffer_current += sprintf(line_buffer_current, "[%02X] ", read(q));
    }
    else
    {
      line_buffer_current += sprintf(line_buffer_current, "%02X ", read(q));
    }
  }

  line_buffer_current += sprintf(line_buffer_current, " -- ZP ");
  for (int i = 0; i < 6; ++i)
    line_buffer_current += sprintf(line_buffer_current, "%02X ", read(i));

  line_buffer_current += sprintf(line_buffer_current, "\n");

  if (in_step_mode)
    printf("%s", line_buffer);

  memcpy(WTF_BUFFER[WTF_PTR], line_buffer, 1024);
  WTF_PTR = (WTF_PTR + 1) % WTF_LEN;
}

void CPU::Disassemble(u16 addr_start, int count, DisassemblyEntry *disassembly_entries)
{
  // TODO : Find actual good start address which aligns with actual instructions

  // Produce disassembly entries
  u16 addr = addr_start;
  for (int entry_i = 0; entry_i < count; entry_i++)
  {
    DisassemblyEntry *dentry = &disassembly_entries[entry_i];

    u8 opcode = read(addr);
    u8 addr1 = read(addr + 1);
    u8 addr2 = read(addr + 2);

    dentry->instruction_bytes[0] = opcode;
    dentry->instruction_bytes[1] = addr1;
    dentry->instruction_bytes[2] = addr2;

    dentry->pc = addr;

    auto opcode_data = instructions[opcode];
    if (opcode_data.addressing == &CPU::addr_implied)
    {
      dentry->num_instruction_bytes = 1;
      sprintf(dentry->buffer, "%s", opcode_data.name.c_str());
    }
    else if (opcode_data.addressing == &CPU::addr_immediate)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s #$%02X", opcode_data.name.c_str(), addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_absolute)
    {
      dentry->num_instruction_bytes = 3;
      sprintf(dentry->buffer, "%s $%02X%02X", opcode_data.name.c_str(), addr2, addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_absolute_x)
    {
      dentry->num_instruction_bytes = 3;
      sprintf(dentry->buffer, "%s $%02X%02X,X", opcode_data.name.c_str(), addr2, addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_absolute_y)
    {
      dentry->num_instruction_bytes = 3;
      sprintf(dentry->buffer, "%s $%02X%02X,Y", opcode_data.name.c_str(), addr2, addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_zeropage)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s $%02X", opcode_data.name.c_str(), addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_zeropage_x)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s $%02X,X", opcode_data.name.c_str(), addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_zeropage_y)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s $%02X,Y", opcode_data.name.c_str(), addr1);
    }
    else if (opcode_data.addressing == &CPU::addr_indirect)
    {
      dentry->num_instruction_bytes = 3;
      sprintf(dentry->buffer, "%s ($%02X%02X)", opcode_data.name.c_str(), addr2, addr1);
      // TODO : show indirect pointer
    }
    else if (opcode_data.addressing == &CPU::addr_indirect_x)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s ($%02X,X)", opcode_data.name.c_str(), addr1);
      // TODO : show indirect pointer
    }
    else if (opcode_data.addressing == &CPU::addr_indirect_y)
    {
      dentry->num_instruction_bytes = 2;
      sprintf(dentry->buffer, "%s ($%02X),Y", opcode_data.name.c_str(), addr1);
      // TODO : show indirect pointer
    }
    else if (opcode_data.addressing == &CPU::addr_relative)
    {
      dentry->num_instruction_bytes = 2;
      u16 branch_address = pc + 2 + sign_extend_16(addr1);
      sprintf(dentry->buffer, "%s $%04X", opcode_data.name.c_str(), branch_address);
    }
    else
    {
      dentry->num_instruction_bytes = 1;
      sprintf(dentry->buffer, "??? opcode 0x%02X", opcode);
    }

    addr += dentry->num_instruction_bytes;
  }
}

void CPU::GetState(State *state)
{
  state->a = a;
  state->x = x;
  state->y = y;
  state->p = p;
  state->s = sp;
  state->pc = pc;
}