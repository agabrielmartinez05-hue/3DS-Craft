#pragma once
// Host adapters for real World.cpp integration tests, not a GPU or libctru emulator.
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <thread>

using LightLock = std::mutex;
inline void LightLock_Init(LightLock*) {}
inline void LightLock_Lock(LightLock* p) {
    p->lock();
}
inline void LightLock_Unlock(LightLock* p) {
    p->unlock();
}
struct LightEvent {
    std::mutex mutex;
    std::condition_variable ready;
    bool signaled{};
};
constexpr int RESET_ONESHOT = 0;
constexpr std::uint64_t U64_MAX = UINT64_MAX;
inline void LightEvent_Init(LightEvent*, int) {}
inline void LightEvent_Signal(LightEvent* event) {
    std::lock_guard<std::mutex> lock(event->mutex);
    event->signaled = true;
    event->ready.notify_one();
}
inline void LightEvent_Wait(LightEvent* event) {
    std::unique_lock<std::mutex> lock(event->mutex);
    event->ready.wait(lock, [&] { return event->signaled; });
    event->signaled = false;
}
using Thread = std::thread*;
inline bool testWorkerThreads = false;
inline Thread threadCreate(void (*entry)(void*), void* argument, std::size_t, int, int, bool) {
    return testWorkerThreads ? new std::thread(entry, argument) : nullptr;
}
inline void threadJoin(Thread t, std::uint64_t) {
    t->join();
}
inline void threadFree(Thread t) {
    delete t;
}
inline std::size_t testLiveAllocations = 0, testTotalAllocations = 0;
inline bool testFailAllocation = false;
inline void* linearAlloc(std::size_t bytes) {
    if (testFailAllocation)
        return nullptr;
    auto p = std::malloc(bytes);
    if (p) {
        ++testLiveAllocations;
        ++testTotalAllocations;
    }
    return p;
}
inline void linearFree(void* p) {
    if (p) {
        --testLiveAllocations;
        std::free(p);
    }
}

inline std::size_t testCacheFlushes = 0;
inline void GSPGPU_FlushDataCache(void*, std::size_t) {
    ++testCacheFlushes;
}
