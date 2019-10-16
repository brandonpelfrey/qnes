#include <functional>
#include <string>

#include "core/cpu.h"
#include "core/bus.h"
#include "core/cpu_state.h"

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
  instructions[0x38] = {"SEC", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0x58] = {"CLI", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0x78] = {"SEI", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0xB8] = {"CLV", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0xD8] = {"CLD", &CPU::CLC, &CPU::addr_implied, 2};
  instructions[0xF8] = {"SED", &CPU::CLC, &CPU::addr_implied, 2};

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
  instructions[0xBD] = {"LDA", &CPU::LDA, &CPU::addr_absolute, 4};
  instructions[0xAD] = {"LDA", &CPU::LDA, &CPU::addr_absolute_x, 4};
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
  instructions[0xEA] = {"NOP", &CPU::NOP, &CPU::addr_implied, 2};

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
  instructions[0x26] = {"ROL", &CPU::ROL, &CPU::addr_implied, 5};
  instructions[0x36] = {"ROL", &CPU::ROL, &CPU::addr_implied, 6};
  instructions[0x2E] = {"ROL", &CPU::ROL, &CPU::addr_implied, 6};
  instructions[0x3E] = {"ROL", &CPU::ROL, &CPU::addr_implied, 7};

  // ROR
  instructions[0x6A] = {"ROR", &CPU::ROR, &CPU::addr_implied, 2};
  instructions[0x66] = {"ROR", &CPU::ROR, &CPU::addr_implied, 5};
  instructions[0x76] = {"ROR", &CPU::ROR, &CPU::addr_implied, 6};
  instructions[0x6E] = {"ROR", &CPU::ROR, &CPU::addr_implied, 6};
  instructions[0x7E] = {"ROR", &CPU::ROR, &CPU::addr_implied, 7};

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
}

////////////////////////////////////////////////////////////////////////////////////////
// CPU Utilities

void CPU::Reset()
{
  pc = 0;
  p = 0x34;
  a = x = y = 0;
  sp = 0xFD;
}

void CPU::SoftReset()
{
  Reset();
  // TODO
}

////////////////////////////////////////////////////////////////////////////////////////
// Instruction Implementations

// This is such a common thing in instructions, that we'll just wrap this into one function.
void CPU::SetNZ(u8 value)
{
  N = value & 0x80 ? 1 : 0;
  Z = value == 0 ? 1 : 0;
}

u8 CPU::ADC()
{
  u16 result = fetched_data + C + a;

  // If both inputs had the same sign but the result has a different sign, then set V.
  bool overflowCheck = (a & 0x80) == (fetched_data & 0x80) && (a & 0x80) != (result & 0x80);

  a = result & 0xFF;
  SetNZ(a);
  C = result > 0xFF ? 1 : 0;
  V = overflowCheck ? 1 : 0;

  return 0;
}

u8 CPU::AND()
{
  a &= fetched_data;
  SetNZ(a);
  return 0;
}

u8 CPU::ASL()
{
  u8 temp = fetched_data;
  C = temp & 0x80 ? 1 : 0;
  temp = (temp << 1) & 0xFF;
  SetNZ(temp);

  if (instructions[opcode].addressing == &CPU::addr_implied)
    a = temp;
  else
    write(addr_abs, temp);
  return 0;
}

u8 CPU::BIT()
{
  u8 temp = a & fetched_data;
  SetNZ(temp);
  V = temp & 0x40 ? 1 : 0;
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
u8 CPU::BPL() { return branchBaseInstruction(N == 0); }

// Branch on 'minus' (negative)
u8 CPU::BMI() { return branchBaseInstruction(N == 1); }

// Branch on overflow clear
u8 CPU::BVC() { return branchBaseInstruction(V == 0); }

// Branch on overflow set
u8 CPU::BVS() { return branchBaseInstruction(V == 1); }

// Branch on carry clear
u8 CPU::BCC() { return branchBaseInstruction(C == 0); }

// Branch on carry set
u8 CPU::BCS() { return branchBaseInstruction(C == 1); }

// Branch on not-equal
u8 CPU::BNE() { return branchBaseInstruction(Z == 0); }

// Branch on equal
u8 CPU::BEQ() { return branchBaseInstruction(Z == 1); }

u8 CPU::BRK() {}
u8 CPU::CMP() {}
u8 CPU::CPX()
{
  u8 temp = x + 1 + ~read(addr_abs);
  SetNZ(temp);
  C = x >= fetched_data ? 1 : 0;
  //printf("CPX temp=%u, C=%u\n", temp, C);
  return 0;
}
u8 CPU::DEC() {}
u8 CPU::EOR() {}

u8 CPU::CLC()
{
  C = 0;
  return 0;
}
u8 CPU::SEC()
{
  C = 1;
  return 0;
}
u8 CPU::CLI()
{
  I = 0;
  return 0;
}
u8 CPU::SEI()
{
  I = 1;
  return 0;
}
u8 CPU::CLV()
{
  V = 0;
  return 0;
}
u8 CPU::CLD()
{
  D = 0;
  return 0;
}
u8 CPU::SED()
{
  D = 1;
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

u8 CPU::JMP() {}
u8 CPU::JSR() {}
u8 CPU::LDA() {}
u8 CPU::LDX() {}
u8 CPU::LDY() {}
u8 CPU::LSR() {}

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

u8 CPU::ROL() {}
u8 CPU::ROR() {}
u8 CPU::RTI() {}
u8 CPU::RTS() {}
u8 CPU::SBC() {}
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
  write(0x0100 | sp, a);
  sp--;
  return 0;
}

u8 CPU::PLA()
{
  sp++;
  a = read(0x0100 | sp);
  SetNZ(a);
  return 0;
}

u8 CPU::PHP()
{
  write(0x0100 | sp, p);
  sp--;
  return 0;
}

u8 CPU::PLP()
{
  sp++;
  p = read(0x0100 | sp);
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
  // ???
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
  u8 high = read(pc++);
  u8 low = read(pc++);

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
  u8 high = read(pc++);
  u8 low = read(pc++);
  addr_abs = (high << 8) | low;

  addr_abs += x;
  bool crossed_page = addr_abs & 0xFF00 != (high << 8);
  return crossed_page ? 1 : 0;
}

u8 CPU::addr_absolute_y()
{
  // Just like absolute, but need to check for page crossing
  u8 high = read(pc++);
  u8 low = read(pc++);
  addr_abs = (high << 8) | low;

  addr_abs += y;
  bool crossed_page = addr_abs & 0xFF00 != (high << 8);
  return crossed_page ? 1 : 0;
}

u8 CPU::addr_indirect() {}
u8 CPU::addr_indirect_x() {}
u8 CPU::addr_indirect_y() {}

u8 CPU::read(u16 addr)
{
  return bus->Read(addr);
}

void CPU::write(u16 addr, u8 val)
{
  bus->Write(addr, val);
}

void CPU::Clock()
{
  if (instruction_remaining_cycles == 0)
  {
    // Read the next instruction
    opcode = read(pc);
    pc++;

    u8 operation_base_cycles = instructions[opcode].cycles;
    u8 address_mode_cycles = (this->*instructions[opcode].addressing)();
    u8 operation_extra_cycles = (this->*instructions[opcode].operation)();

    instruction_remaining_cycles = operation_base_cycles + address_mode_cycles + operation_extra_cycles;
  }

  instruction_remaining_cycles--;
  total_clock_cycles++;

  // Invoke
}

void CPU::Step()
{
  // Finish any remaining instruction which was already in flight
  while (instruction_remaining_cycles > 0)
    Clock();

  // Start the next instruction, then run it to completion
  Clock();
  while (instruction_remaining_cycles > 0)
    Clock();
}

void CPU::Debug()
{
  printf("0x%04X ", pc);

  u8 opcode = read(pc);
  auto entry = instructions[opcode];

  if (entry.addressing == &CPU::addr_implied)
  {
    printf("%s", entry.name.c_str());
  }
  else if (entry.addressing == &CPU::addr_immediate)
  {
    printf("%s $%02X", entry.name.c_str(), read(pc + 1));
  }
  else if (entry.addressing == &CPU::addr_relative)
  {
    u16 branch_address = pc + 2 + sign_extend_16(read(pc + 1));
    printf("%s 0x%04X", entry.name.c_str(), branch_address);
  }
  else
  {
    printf("??? opcode 0x%02X", opcode);
  }
  printf("\n");

  //printf("%010u :: %s --  A=0x%02X X=0x%02X Y=0x%02X Z=%d PC=0x%04X\n", total_clock_cycles, instructions[opcode].name.c_str(), a, x, y, Z, pc);
}
