// Validator CLI: runs the core's trace reader over a captured trace file.
//
// The reader is a library (harness/include/mh/trace_reader.hpp); without this command a
// captured run could only be graded by writing a test. Exit codes are the contract:
//
//   0  structurally valid (no error-severity issue)
//   1  invalid: the reader reported at least one error
//   2  usage or I/O problem (nothing was read)
//
// `--json` prints the same findings as one JSON object, so the runner can consume them
// without parsing English. The wording of `detail` is the reader's, not this tool's.

#include <mh/json.hpp>
#include <mh/trace_reader.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr char kUsage[] =
    "usage: mh_trace_check <trace.jsonl> [--json]\n"
    "  exit 0 structurally valid, 1 invalid, 2 usage or I/O error\n";

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

std::string_view severity_name(mh::TraceIssueSeverity severity) {
    return severity == mh::TraceIssueSeverity::error ? "error" : "warning";
}

void print_human(const std::string& path, const mh::TraceDocument& document) {
    std::size_t errors = 0;
    std::size_t warnings = 0;
    for (const mh::TraceIssue& issue : document.issues) {
        if (issue.severity == mh::TraceIssueSeverity::error) {
            ++errors;
        } else {
            ++warnings;
        }
        std::cout << (issue.line ? "line " + std::to_string(issue.line) : std::string("document"))
                  << ": " << severity_name(issue.severity) << ": " << issue.kind;
        if (!issue.detail.empty()) {
            std::cout << ": " << issue.detail;
        }
        std::cout << "\n";
    }
    std::cout << path << ": " << (document.structurally_valid() ? "valid" : "invalid") << ", "
              << document.events.size() << " event(s), " << errors << " error(s), " << warnings
              << " warning(s), meta " << (document.meta ? "present" : "missing") << ", end "
              << (document.end ? "present" : "missing") << "\n";
}

void print_json(const std::string& path, const mh::TraceDocument& document) {
    mh::json::Json issues = mh::json::Json::array();
    std::size_t errors = 0;
    std::size_t warnings = 0;
    for (const mh::TraceIssue& issue : document.issues) {
        if (issue.severity == mh::TraceIssueSeverity::error) {
            ++errors;
        } else {
            ++warnings;
        }
        mh::json::Json entry = mh::json::Json::object();
        entry.set("severity", mh::json::Json::string(severity_name(issue.severity)));
        entry.set("kind", mh::json::Json::string(mh::to_string(issue.kind)));
        entry.set("line", mh::json::Json::integer(static_cast<std::int64_t>(issue.line)));
        entry.set("detail", mh::json::Json::string(issue.detail));
        issues.push(std::move(entry));
    }

    mh::json::Json report = mh::json::Json::object();
    report.set("trace", mh::json::Json::string(path));
    report.set("structurally_valid", mh::json::Json::boolean(document.structurally_valid()));
    report.set("meta", mh::json::Json::boolean(document.meta.has_value()));
    report.set("end", mh::json::Json::boolean(document.end.has_value()));
    report.set("events", mh::json::Json::integer(static_cast<std::int64_t>(document.events.size())));
    report.set("errors", mh::json::Json::integer(static_cast<std::int64_t>(errors)));
    report.set("warnings", mh::json::Json::integer(static_cast<std::int64_t>(warnings)));
    report.set("issues", std::move(issues));
    std::cout << report.to_json() << "\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string path;
    bool as_json = false;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--json") {
            as_json = true;
        } else if (argument == "-h" || argument == "--help") {
            std::cout << kUsage;
            return 0;
        } else if (!argument.empty() && argument.front() == '-') {
            std::cerr << "unknown option: " << argument << "\n" << kUsage;
            return 2;
        } else if (path.empty()) {
            path = argument;
        } else {
            std::cerr << "only one trace may be checked at a time\n" << kUsage;
            return 2;
        }
    }
    if (path.empty()) {
        std::cerr << kUsage;
        return 2;
    }

    std::string text = read_file(path);
    if (text.empty()) {
        std::cerr << "cannot read trace or it is empty: " << path << "\n";
        return 2;
    }

    const mh::TraceDocument document = mh::read_trace(text);
    if (as_json) {
        print_json(path, document);
    } else {
        print_human(path, document);
    }
    return document.structurally_valid() ? 0 : 1;
}
