#include "Metrics.hpp"
#include <3ds.h>
#include <malloc.h>

extern "C" {
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap_size;
}
namespace voxel {
void PerformanceMetrics::frame(double elapsed) {
    fps_.frame(elapsed);
    sampleElapsed_ += elapsed;
    if (sampleElapsed_ < 0.5)
        return;
    sampleElapsed_ = 0;
    const auto heap = mallinfo();
    // Kernel process-commit counters include libctru's pre-reserved heaps and do
    // not show chunk allocation/free activity. Report live allocator bytes instead.
    live_ = static_cast<std::size_t>(heap.uordblks) + __ctru_linear_heap_size - linearSpaceFree();
    reserved_ = __ctru_heap_size + __ctru_linear_heap_size;
}
} // namespace voxel
