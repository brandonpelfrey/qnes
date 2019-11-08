#pragma once

#include <algorithm>
#include <vector>
#include "core/types.h"

class CPUDebugging
{
private:
  std::vector<u16> execute_breakpoints;
  std::vector<u16> write_breakpoints;
  std::vector<u16> read_breakpoints;

  std::vector<u16>::iterator get_execution_breakpoint(u16 addr)
  {
    return std::find(execute_breakpoints.begin(), execute_breakpoints.end(), addr);
  }

public:
  void AddExecutionBreakpoint(u16 addr)
  {
    if (get_execution_breakpoint(addr) == execute_breakpoints.end())
      execute_breakpoints.push_back(addr);
  }
  bool HasExecutionBreakpoint(u16 addr)
  {
    return get_execution_breakpoint(addr) != execute_breakpoints.end();
  }
  void RemoveExecutionBreakpoint(u16 addr)
  {
    if (auto it = get_execution_breakpoint(addr); it != execute_breakpoints.end())
      execute_breakpoints.erase(it);
  }
};