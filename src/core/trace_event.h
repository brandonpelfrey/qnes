#pragma once
#include <chrono>

#include "./types.h"

struct TraceEvent
{
  u64 start;
  TraceEvent()
  {
    using namespace std::chrono;
    auto now = system_clock::now().time_since_epoch();
    start = duration_cast<nanoseconds>(now).count();
  }
};

class TraceEventEmitter
{
private:
  TraceEventEmitter();
  ~TraceEventEmitter();

public:
  static TraceEventEmitter *Instance();
  static void Emit(const TraceEvent &event, const char *name);
};