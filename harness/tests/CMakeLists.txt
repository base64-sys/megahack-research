add_executable(mh_trace_tests
  test_main.cpp
  test_contract.cpp
  test_value.cpp
  test_json.cpp
  test_json_parse.cpp
  test_sha256.cpp
  test_fingerprint.cpp
  test_trace_writer.cpp
  test_call_stack.cpp
  test_trace_reader.cpp
  test_scenario.cpp
  test_comparator.cpp
  test_sequence.cpp
  test_comparison.cpp
  test_corpus.cpp
  test_context.cpp
  test_window.cpp
  test_records.cpp
  test_compare.cpp
)

target_link_libraries(mh_trace_tests PRIVATE mh_trace)
target_compile_features(mh_trace_tests PRIVATE cxx_std_20)
target_compile_definitions(mh_trace_tests PRIVATE
  MH_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

if(MSVC)
  target_compile_options(mh_trace_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(mh_trace_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME mh_trace_tests COMMAND mh_trace_tests)
set_tests_properties(mh_trace_tests PROPERTIES TIMEOUT 60)

