#pragma once

#include <unistd.h>

#include <utility>

// UniqueFd is the only owner of one file descriptor.
class UniqueFd {
public:
    UniqueFd() noexcept = default;

    explicit UniqueFd(int fd) noexcept
        : fd_(fd) {
    }

    ~UniqueFd() {
        // A destructor cannot report failure by throwing.
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {
    }

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    int get() const noexcept {
        return fd_;
    }

    explicit operator bool() const noexcept {
        return fd_ != -1;
    }

    int release() noexcept {
        return std::exchange(fd_, -1);
    }

    void reset(int new_fd = -1) noexcept {
        if (fd_ == new_fd) {
            return;
        }
        if (fd_ != -1) {
            ::close(fd_);
        }
        fd_ = new_fd;
    }

private:
    int fd_ = -1;
};