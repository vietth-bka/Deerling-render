#include <lightwave/profiler.hpp>

namespace lightwave {

GlobalProfiler globalProfiler;
thread_local ThreadProfiler threadProfiler;

} // namespace lightwave
