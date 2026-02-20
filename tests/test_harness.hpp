#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace testh {

using TestFn = std::function<void()>;

struct TestCase {
  std::string name;
  TestFn fn;
};

inline std::vector<TestCase> &Registry() {
  static std::vector<TestCase> tests;
  return tests;
}

inline void Register(const std::string &name, TestFn fn) {
  Registry().push_back({name, std::move(fn)});
}

inline int RunAll() {
  int failed = 0;
  for (const auto &t : Registry()) {
    try {
      t.fn();
      std::cout << "[PASS] " << t.name << "\n";
    } catch (const std::exception &ex) {
      ++failed;
      std::cerr << "[FAIL] " << t.name << ": " << ex.what() << "\n";
    }
  }
  return failed;
}

} // namespace testh

#define TEST_CASE(name)                                                                            \
  static void name();                                                                              \
  namespace {                                                                                      \
  struct name##_registrar {                                                                        \
    name##_registrar() { testh::Register(#name, &name); }                                          \
  } name##_registrar_instance;                                                                     \
  }                                                                                                \
  static void name()

#define REQUIRE(cond)                                                                              \
  do {                                                                                             \
    if (!(cond))                                                                                   \
      throw std::runtime_error(std::string("Requirement failed: ") + #cond);                       \
  } while (false)

#define REQUIRE_NEAR(a, b, eps)                                                                    \
  do {                                                                                             \
    if (std::fabs((a) - (b)) > (eps))                                                              \
      throw std::runtime_error("Near check failed");                                               \
  } while (false)
