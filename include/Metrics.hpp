#pragma once
#include "Gameplay.hpp"
#include <cstddef>

namespace voxel {
class PerformanceMetrics {
  public:
    void frame(double elapsed);
    float fps() const { return fps_.fps(); }
    std::size_t liveBytes() const { return live_; }
    std::size_t reservedBytes() const { return reserved_; }

  private:
    FpsCounter fps_;
    double sampleElapsed_ = 1.0;
    std::size_t live_{}, reserved_{};
};
} // namespace voxel
