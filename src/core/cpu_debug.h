#pragma once

#include <algorithm>
#include <vector>
#include "core/types.h"

#include <unordered_map>
#include <optional>

struct Breakpoint
{
  u16 addr;
  u8 mask;

  static const u8 READ = 1;
  static const u8 WRITE = 2;
  static const u8 EXECUTE = 4;

  Breakpoint() {}
  Breakpoint(u16 addr) : addr(addr), mask(EXECUTE) {}
  Breakpoint(u16 addr, u8 flags) : addr(addr), mask(flags) {}
};

class CPUDebugging
{
private:
  using BreakpointMap = std::unordered_map<u16, Breakpoint>;
  BreakpointMap breakpoints;

public:
  void Add(Breakpoint bp)
  {
    breakpoints[bp.addr] = bp;
  }

  void Remove(u16 addr)
  {
    breakpoints.erase(addr);
  }

  void Remove(Breakpoint bp)
  {
    Remove(bp.addr);
  }

  const BreakpointMap &GetAll()
  {
    return breakpoints;
  }

  bool Has(u16 addr, u8 flag_filter = Breakpoint::EXECUTE)
  {
    auto it = breakpoints.find(addr);
    return (it != breakpoints.end()) && (it->second.mask & flag_filter);
  }
};