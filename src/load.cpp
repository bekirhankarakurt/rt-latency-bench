#include "rtlb/load.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace rtlb {
namespace {

// Scratch file removed on destruction; the descriptor is closed first.
class TempFile {
public:
    explicit TempFile(std::filesystem::path path)
        : path_(std::move(path)), fd_(::open(path_.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600)) {
        if (fd_ < 0) {
            throw std::runtime_error("cannot create IO load file " + path_.string() + ": " +
                                     std::generic_category().message(errno));
        }
    }
    ~TempFile() {
        if (fd_ >= 0) {
            ::close(fd_);
            std::error_code ignored;
            std::filesystem::remove(path_, ignored);
        }
    }
    TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)), fd_(other.fd_) {
        other.fd_ = -1;
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    [[nodiscard]] int fd() const noexcept { return fd_; }

private:
    std::filesystem::path path_;
    int fd_;
};

void cpu_load(const std::atomic<bool>& stop) {
    std::uint64_t x = 0x2545F4914F6CDD1DULL;
    std::atomic<std::uint64_t> sink{0};
    while (!stop.load(std::memory_order_relaxed)) {
        for (int i = 0; i < 100000; ++i) {
            x = (x * 6364136223846793005ULL) + 1442695040888963407ULL;
        }
        sink.store(x, std::memory_order_relaxed);
    }
}

void memory_load(const std::atomic<bool>& stop, unsigned mib) {
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(mib) << 20, 1);
    constexpr std::size_t kCacheLine = 64;
    while (!stop.load(std::memory_order_relaxed)) {
        for (std::size_t i = 0; i < buffer.size(); i += kCacheLine) {
            buffer[i] = static_cast<std::uint8_t>(buffer[i] + 1);
        }
    }
}

void io_load(const std::atomic<bool>& stop, const TempFile& file) {
    const std::vector<char> block(1 << 20, 'x');
    while (!stop.load(std::memory_order_relaxed)) {
        if (::lseek(file.fd(), 0, SEEK_SET) < 0) return;
        for (int i = 0; i < 4 && !stop.load(std::memory_order_relaxed); ++i) {
            if (::write(file.fd(), block.data(), block.size()) < 0) return;
        }
        if (::fsync(file.fd()) < 0) return;
    }
}

}  // namespace

LoadSet::LoadSet(const Config& config) {
    try {
        std::size_t io_index = 0;
        const std::filesystem::path dir =
            config.io_dir.empty() ? std::filesystem::temp_directory_path()
                                : std::filesystem::path(config.io_dir);
        for (const LoadKind kind : config.loads) {
            for (unsigned n = 0; n < config.load_threads; ++n) {
                switch (kind) {
                    case LoadKind::Cpu:
                        threads_.emplace_back(cpu_load, std::cref(stop_));
                        break;
                    case LoadKind::Memory:
                        threads_.emplace_back(memory_load, std::cref(stop_), config.mem_mib);
                        break;
                    case LoadKind::Io: {
                        TempFile file(dir / ("rtlb-io-" + std::to_string(::getpid()) + "-" +
                                             std::to_string(io_index++) + ".tmp"));
                        threads_.emplace_back(
                            [this](TempFile owned) { io_load(stop_, owned); }, std::move(file));
                        break;
                    }
                }
            }
        }
    } catch (...) {
        stop();
        throw;
    }
}

LoadSet::~LoadSet() { stop(); }

void LoadSet::stop() noexcept {
    stop_.store(true, std::memory_order_relaxed);
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

}  // namespace rtlb
