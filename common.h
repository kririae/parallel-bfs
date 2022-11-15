#pragma once

#include <chrono>

#include "spdlog/spdlog.h"

#define Info(...)              \
  do {                         \
    spdlog::info(__VA_ARGS__); \
  } while (false)

#define Warn(...)              \
  do {                         \
    spdlog::warn(__VA_ARGS__); \
  } while (false)

#define Critical(...)              \
  do {                             \
    spdlog::critical(__VA_ARGS__); \
  } while (false)

#define Error(...)              \
  do {                          \
    spdlog::error(__VA_ARGS__); \
  } while (false)

#define FORCEINLINE __always_inline

struct TimeInfo {
  std::size_t ms;
  std::string desc;
};

struct Event {
  using clock = std::chrono::high_resolution_clock;
  Event() : m_start(clock::now()) {}
  float end() const {
    using namespace std::literals::chrono_literals;
    return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() -
                                                                 m_start)
               .count() /
           1000.0;
  }

private:
  decltype(clock::now()) m_start;
};

struct Solution {
  std::vector<int> distance;
  std::vector<int> parent;
};