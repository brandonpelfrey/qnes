#include "./trace_event.h"
#include <cstdio>

TraceEventEmitter *instance = nullptr;
FILE *trace_file = nullptr;

TraceEventEmitter::TraceEventEmitter()
{
#ifdef TRACE_EVENT_PROFILING
  trace_file = fopen("./trace.json", "w");
  fprintf(trace_file, "[\n");
#endif
}

TraceEventEmitter::~TraceEventEmitter()
{
#ifdef TRACE_EVENT_PROFILING
  fclose(trace_file);
#endif
}

TraceEventEmitter *TraceEventEmitter::Instance()
{
  if (!instance)
    instance = new TraceEventEmitter();
  return instance;
}

void TraceEventEmitter::Emit(const TraceEvent &event, const char *name)
{
#ifdef TRACE_EVENT_PROFILING
  u64 end = event.end;
  if (end == 0)
    end = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  double st = event.start / 1000.0;
  double dur = (end - event.start) / 1000.0;
  if (dur < 1)
    dur = 1;

  fprintf(trace_file,
          "{ \"pid\":1, \"tid\":1, \"ts\":%.3lf, \"dur\":%.3lf, \"ph\":\"X\", \"name\":\"%s\", \"args\":{} },\n",
          st, dur, name);
#endif
}
