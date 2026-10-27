// Item 1 — skeleton: the schema strings are the contract's, verbatim.
// notes/runtime-validation.md §Trace format, §Scenario manifest.

#include <string_view>

#include "mh/contract.hpp"
#include "test_runner.hpp"

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
