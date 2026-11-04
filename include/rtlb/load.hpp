#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "rtlb/config.hpp"

namespace rtlb {

/// RAII set of background load generators (CPU spin, memory sweep, file IO).
/// Threads start in the constructor and are stopped and joined in the destructor.
class LoadSet {
public:
    explicit LoadSet(const Config& config);
    ~LoadSet();

    LoadSet(const LoadSet&) = delete;
    LoadSet& operator=(const LoadSet&) = delete;
    LoadSet(LoadSet&&) = delete;
    LoadSet& operator=(LoadSet&&) = delete;

    [[nodiscard]] std::size_t thread_count() const noexcept { return threads_.size(); }

private:
    void stop() noexcept;

    std::atomic<bool> stop_{false};
    std::vector<std::thread> threads_;
};

}  // namespace rtlb
