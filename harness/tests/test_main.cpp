// Test runner for the trace core.
//
// Own runner, no third-party framework (the harness must not depend on one). Each
// translation unit registers tests with MH_TEST; main() runs them in registration order
// and exits non-zero if any check failed, so ctest and a plain invocation agree.

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "mh/contract.hpp"

namespace {

using TestFn = void (*)();

struct TestCase {
  const char* name;
  TestFn fn;
};

std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

// Failures recorded by the test currently running.
int g_failures = 0;

struct Registrar {
  Registrar(const char* name, TestFn fn) { registry().push_back(TestCase{name, fn}); }
};

void record_failure(const char* file, int line, const std::string& what) {
  ++g_failures;
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

} // namespace

#define MH_TEST(test_name)                                                    \
  static void test_name();                                                    \
  static ::Registrar registrar_##test_name(#test_name, &test_name);           \
  static void test_name()

#define MH_CHECK(condition)                                                   \
  do {                                                                        \
    if (!(condition)) {                                                       \
      ::record_failure(__FILE__, __LINE__, "expected: " #condition);           \
    }                                                                         \
  } while (false)

#define MH_CHECK_EQ(lhs, rhs)                                                 \
  ::check_equal((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)

// ---------------------------------------------------------------------------
// Item 1 — skeleton: the schema strings are the contract's, verbatim.
// notes/runtime-validation.md §Trace format, §Scenario manifest.
// ---------------------------------------------------------------------------

MH_TEST(schema_strings_match_contract) {
  MH_CHECK_EQ(mh::contract::kTraceSchema, std::string_view{"mh-trace/1"});
  MH_CHECK_EQ(mh::contract::kScenarioSchema, std::string_view{"mh-scenario/1"});
  MH_CHECK_EQ(mh::contract::kHarnessId, std::string_view{"megahack.trace"});
  MH_CHECK_EQ(mh::contract::kHarnessVersion, std::string_view{"0.1.0"});
}

MH_TEST(record_type_round_trips) {
  using mh::contract::RecordType;
  MH_CHECK_EQ(std::string_view{mh::contract::kTypeMeta}, std::string_view{"meta"});
  MH_CHECK_EQ(std::string_view{mh::contract::kTypeEvent}, std::string_view{"event"});
  MH_CHECK_EQ(std::string_view{mh::contract::kTypeEnd}, std::string_view{"end"});

  const RecordType all[] = {RecordType::meta, RecordType::event, RecordType::end};
  for (RecordType type : all) {
    const auto parsed = mh::contract::parse_record_type(mh::contract::to_string(type));
    MH_CHECK(parsed.has_value());
    MH_CHECK(parsed == type);
  }
}

MH_TEST(record_type_rejects_unknown_spelling) {
  MH_CHECK(!mh::contract::parse_record_type("Meta").has_value());
  MH_CHECK(!mh::contract::parse_record_type("").has_value());
  MH_CHECK(!mh::contract::parse_record_type("mh-trace/2").has_value());
}

int main() {
  int failed_tests = 0;
  for (const TestCase& test : registry()) {
    g_failures = 0;
    test.fn();
    if (g_failures == 0) {
      std::printf("ok    %s\n", test.name);
    } else {
      ++failed_tests;
      std::printf("FAIL  %s\n", test.name);
    }
  }

  const int total = static_cast<int>(registry().size());
  std::printf("%d test(s), %d failed\n", total, failed_tests);
  return failed_tests == 0 ? 0 : 1;
}
