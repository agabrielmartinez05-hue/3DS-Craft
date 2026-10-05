#pragma once
#include <cstdarg>
#include <cstdio>
#include <exception>

namespace voxel {
// No heap allocation: still usable after std::bad_alloc or linearAlloc failure.
class Error final : public std::exception {
  public:
    explicit Error(const char* format, ...) noexcept {
        va_list args;
        va_start(args, format);
        std::vsnprintf(message_, sizeof(message_), format, args);
        va_end(args);
    }
    const char* what() const noexcept override { return message_; }

  private:
    char message_[768]{};
};
} // namespace voxel
