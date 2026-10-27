// Test registration shared by every test translation unit.
//
// Own runner, no third-party framework (the harness must not depend on one). Each file
// registers tests with MH_TEST; main() (test_main.cpp) runs them in registration order
// and exits non-zero if any check failed, so ctest and a plain invocation agree.

#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace mh::test {

using TestFn = void (*)();

struct TestCase {
  const char* name;
  TestFn fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

// Failures recorded by the test currently running.
inline int& failures() {
  static int count = 0;
  return count;
}

struct Registrar {
  Registrar(const char* name, TestFn fn) { registry().push_back(TestCase{name, fn}); }
};

inline void record_failure(const char* file, int line, const std::string& what) {
  ++failures();
  std::printf("    %s:%d: %s\n", file, line, what.c_str());
}

template <typename A, typename B>
void check_equal(const A& lhs, const B& rhs, const char* lhs_expr, const char* rhs_expr,
                 const char* file, int line) {
  if (lhs == rhs) {
    return;
  }
  std::ostringstream message;
  message << lhs_expr << " == " << rhs_expr << " (got " << lhs << " vs " << rhs << ")";
  record_failure(file, line, message.str());
}

} // namespace mh::test

#define MH_TEST(test_name)                                                    \
  static void test_name();                                                    \
  static ::mh::test::Registrar registrar_##test_name(#test_name, &test_name);  \
  static void test_name()

#define MH_CHECK(condition)                                                   \
  do {                                                                        \
    if (!(condition)) {                                                       \
      ::mh::test::record_failure(__FILE__, __LINE__, "expected: " #condition); \
    }                                                                         \
  } while (false)

#define MH_CHECK_EQ(lhs, rhs)                                                 \
  ::mh::test::check_equal((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)
