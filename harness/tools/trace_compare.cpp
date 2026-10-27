// Compare a manifest's two variants and print the result status.
//
// `compare_traces` decides everything; this only reads files and reports. Exit code follows the
// other tools: 0 when the status is pass or pass_with_warnings, 1 when it is fail, blocked,
// unstable or invalid, and 2 for usage. A blocked comparison is not a tool failure - it means
// the evidence was not there - so the printed status is what matters and the exit code is only
// for scripts.

#include "mh/compare.hpp"
#include "mh/scenario.hpp"
#include "mh/trace_reader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool read_file(const std::string& path, std::string& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    out = buffer.str();
    return true;
}

void usage() {
    std::cerr << "usage: mh_trace_compare <manifest.json> --release <trace.jsonl> [--release ...]"
                 " [--reconstruction <trace.jsonl> ...]\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        usage();
        return 2;
    }

    std::string manifest_text;
    if (!read_file(argv[1], manifest_text)) {
        std::cerr << "cannot read manifest " << argv[1] << "\n";
        return 2;
    }
    const mh::ScenarioDocument manifest = mh::read_scenario(manifest_text);

    std::vector<mh::TraceDocument> release_runs;
    std::vector<mh::TraceDocument> reconstruction_runs;
    for (int index = 2; index + 1 < argc; index += 2) {
        const std::string variant = argv[index];
        const std::string path = argv[index + 1];
        std::string text;
        if (!read_file(path, text)) {
            std::cerr << "cannot read trace " << path << "\n";
            return 2;
        }
        mh::TraceDocument document = mh::read_trace(text);
        for (const mh::TraceIssue& issue : document.issues) {
            if (issue.severity == mh::TraceIssueSeverity::error) {
                std::cerr << path << ": " << mh::to_string(issue.kind) << ": " << issue.detail
                          << "\n";
            }
        }
        if (variant == "--release") {
            release_runs.push_back(std::move(document));
        }
        else if (variant == "--reconstruction") {
            reconstruction_runs.push_back(std::move(document));
        }
        else {
            usage();
            return 2;
        }
    }

    const mh::ComparisonReport report =
        mh::compare_traces(manifest, release_runs, reconstruction_runs);

    std::cout << "status: " << mh::to_string(report.status)
              << " | gate: " << mh::to_string(report.gate)
              << " | unstable: " << (report.unstable ? "yes" : "no") << "\n";
    for (const mh::Finding& finding : report.findings) {
        std::cout << "  " << mh::to_string(finding.severity) << " " << finding.check << ": "
                  << finding.detail << "\n";
    }

    switch (report.status) {
        case mh::ComparisonStatus::pass:
        case mh::ComparisonStatus::pass_with_warnings:
            return 0;
        default:
            return 1;
    }
}
