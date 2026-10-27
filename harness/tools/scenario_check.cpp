// Check a `mh-scenario/1` manifest and report what it says.
//
// Exit codes follow mh_trace_check: 0 valid, 1 invalid, 2 usage. A valid draft manifest exits 0
// but says it is not evidence eligible, because "only `approved` scenarios may produce M3
// evidence" and silently treating draft as approved is how a draft becomes evidence by accident.

#include "mh/scenario.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: mh_scenario_check <manifest.json>\n";
        return 2;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "cannot read " << argv[1] << "\n";
        return 2;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();

    const mh::ScenarioDocument document = mh::read_scenario(buffer.str());
    for (const mh::ScenarioIssue& issue : document.issues) {
        std::cout << (issue.severity == mh::ScenarioIssueSeverity::error ? "error  " : "warning")
                  << " " << issue.path << ": " << issue.detail << "\n";
    }

    if (document.has_errors() || !document.scenario) {
        std::cout << argv[1] << ": invalid, " << document.issues.size() << " issue(s)\n";
        return 1;
    }

    const mh::Scenario& scenario = *document.scenario;
    std::cout << argv[1] << ": valid, id " << scenario.id << ", status "
              << mh::to_string(scenario.status) << ", sha256 " << document.sha256 << ", "
              << document.issues.size() << " issue(s)";
    if (!document.approved_for_evidence()) {
        std::cout << ", not evidence eligible";
    }
    std::cout << "\n";
    return 0;
}
