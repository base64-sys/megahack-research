#include "hackpro/extensions.hpp"
#include "hackpro/config.hpp"
#include "geode/entry.hpp"
#include <cassert>
#include <iostream>
#include <vector>

void test_entry_point() {
    geodeImplicitEntry();
    auto* mod = hackpro::getMod();
    assert(mod != nullptr);
    std::cout << "ok    geodeImplicitEntry runs and caches mod pointer\n";
}

void test_key_tag_mapping() {
    using namespace hackpro::extensions;

    auto tag1 = tagFor("HOME/SPEEDHACK");
    assert(!tag1.has_value());

    // Setting a variable interns the key
    setConfigVar<bool>("HOME/SPEEDHACK", true);
    auto tag2 = tagFor("HOME/SPEEDHACK");
    assert(tag2.has_value());
    assert(tag2.value() != hackpro::INVALID_TAG);

    auto key = keyFor(tag2.value());
    assert(key == "HOME/SPEEDHACK");

    std::cout << "ok    key/tag interning and round-trip resolution\n";
}

void test_typed_config_vars() {
    using namespace hackpro::extensions;

    // 1. bool / hack enable
    setConfigVar<bool>("test/bool", true);
    assert(getConfigVar<bool>("test/bool") == true);
    assert(isHackEnabled("test/bool") == true);
    setHackEnabled("test/bool", false);
    assert(isHackEnabled("test/bool") == false);
    assert(getConfigVar<bool>("test/bool") == false);

    // 2. double
    setConfigVar<double>("test/double", 3.14159);
    assert(getConfigVar<double>("test/double") == 3.14159);

    // 3. int64_t
    setConfigVar<int64_t>("test/int64", 42000000000LL);
    assert(getConfigVar<int64_t>("test/int64") == 42000000000LL);

    // 4. std::string
    setConfigVar<std::string>("test/string", "MegaHack");
    assert(getConfigVar<std::string>("test/string") == "MegaHack");

    // 5. std::vector<unsigned int> (Bytes map)
    std::vector<unsigned int> bytes = { 0xDE, 0xAD, 0xBE, 0xEF };
    setConfigVar<std::vector<unsigned int>>("test/bytes", bytes);
    auto retrievedBytes = getConfigVar<std::vector<unsigned int>>("test/bytes");
    assert(retrievedBytes.size() == 4);
    assert(retrievedBytes[0] == 0xDE && retrievedBytes[3] == 0xEF);

    std::cout << "ok    all 5 exported config var types (bool, double, int64, string, bytes)\n";
}

void test_listeners() {
    using namespace hackpro::extensions;

    bool boolFired = false;
    registerListener<bool>("test/bool_listen", [&](hackpro::Tag tag, bool const& val) {
        boolFired = val;
    });
    setConfigVar<bool>("test/bool_listen", true);
    assert(boolFired == true);

    bool hackListenerFired = false;
    registerHackListener("test/hack_listen", [&](hackpro::Tag tag, bool const& val) {
        hackListenerFired = val;
    });
    setHackEnabled("test/hack_listen", true);
    assert(hackListenerFired == true);

    double doubleVal = 0.0;
    registerListener<double>("test/double_listen", [&](hackpro::Tag tag, double const& val) {
        doubleVal = val;
    });
    setConfigVar<double>("test/double_listen", 2.718);
    assert(doubleVal == 2.718);

    int64_t intVal = 0;
    registerListener<int64_t>("test/int_listen", [&](hackpro::Tag tag, int64_t const& val) {
        intVal = val;
    });
    setConfigVar<int64_t>("test/int_listen", 1337);
    assert(intVal == 1337);

    std::string strVal;
    registerListener<std::string>("test/str_listen", [&](hackpro::Tag tag, std::string const& val) {
        strVal = val;
    });
    setConfigVar<std::string>("test/str_listen", "reconstructed");
    assert(strVal == "reconstructed");

    std::vector<unsigned int> byteVal;
    registerListener<std::vector<unsigned int>>("test/byte_listen", [&](hackpro::Tag tag, std::vector<unsigned int> const& val) {
        byteVal = val;
    });
    setConfigVar<std::vector<unsigned int>>("test/byte_listen", { 1, 2, 3 });
    assert(byteVal.size() == 3);

    std::cout << "ok    typed change listeners and hack listeners dispatch correctly\n";
}

void test_actions_and_definitions() {
    using namespace hackpro::extensions;

    bool actionRan = false;
    registerActionListener("IMPORT_DT", [&](hackpro::Tag tag) {
        actionRan = true;
    });
    hackpro::Config::instance().triggerAction("IMPORT_DT");
    assert(actionRan == true);

    addDefinition("ICONIC/SPEED", "Custom speed definition");
    assert(hackpro::define("ICONIC/SPEED") == "Custom speed definition");

    assert(mega::utils::safe_mode::areCheatsEnabled() == true);
    mega::utils::safe_mode::setCheatsEnabled(false);
    assert(mega::utils::safe_mode::areCheatsEnabled() == false);
    mega::utils::safe_mode::setCheatsEnabled(true);

    std::cout << "ok    action listeners, definitions, and safe_mode are functional\n";
}

void test_serialization() {
    auto json = hackpro::Config::instance().serializeJson();
    assert(json.find("\"version\": 1") != std::string::npos);
    assert(json.find("\"hacks\":") != std::string::npos);
    assert(json.find("HOME/SPEEDHACK") != std::string::npos);
    assert(hackpro::Config::instance().deserializeJson(json) == true);
    std::cout << "ok    settings registry JSON serialization produces valid hackpro.json schema\n";
}

int main() {
    std::cout << "=== Running Mega Hack Reconstructed Core Tests ===\n";
    test_entry_point();
    test_key_tag_mapping();
    test_typed_config_vars();
    test_listeners();
    test_actions_and_definitions();
    test_serialization();
    std::cout << "All core infrastructure reconstruction tests passed successfully!\n";
    return 0;
}
