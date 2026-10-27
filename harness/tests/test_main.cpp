// Entry point for the trace-core test suite. Test bodies and registration live in
// per-item translation units (test_*.cpp) and use the MH_* macros from test_runner.hpp.

#include <cstdio>

#include "test_runner.hpp"

int main() {
  int failed_tests = 0;
  for (const mh::test::TestCase& test : mh::test::registry()) {
    mh::test::failures() = 0;
    test.fn();
    if (mh::test::failures() == 0) {
      std::printf("ok    %s\n", test.name);
    } else {
      ++failed_tests;
      std::printf("FAIL  %s\n", test.name);
    }
  }

  const int total = static_cast<int>(mh::test::registry().size());
  std::printf("%d test(s), %d failed\n", total, failed_tests);
  return failed_tests == 0 ? 0 : 1;
}
