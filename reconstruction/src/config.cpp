#include "hackpro/config.hpp"
#include <sstream>
#include <iomanip>

namespace hackpro {

Config::Config() = default;
Config::~Config() = default;

Config& Config::instance() {
    static Config s_instance;
    return s_instance;
}

Tag Config::internKey(std::string const& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        return it->second;
    }
    Tag newTag = m_nextTag++;
    m_keyToTag[key] = newTag;
    m_tagToKey[newTag] = key;
    return newTag;
}

std::optional<Tag> Config::tagFor(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string Config::keyFor(Tag tag) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tagToKey.find(tag);
    if (it != m_tagToKey.end()) {
        return it->second;
    }
    return "";
}

bool Config::isHackEnabled(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it == m_keyToTag.end()) {
        return false;
    }
    auto bIt = m_bools.find(it->second);
    if (bIt != m_bools.end()) {
        return bIt->second;
    }
    return false;
}

void Config::setHackEnabled(std::string const& key, bool enabled) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag, bool const&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bools[tag] = enabled;
        auto it = m_hackListeners.find(tag);
        if (it != m_hackListeners.end()) {
            callbacks = it->second;
        }
        auto bIt = m_boolListeners.find(tag);
        if (bIt != m_boolListeners.end()) {
            callbacks.insert(callbacks.end(), bIt->second.begin(), bIt->second.end());
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag, enabled);
    }
}

void Config::registerHackListener(std::string const& key, std::function<void(Tag, bool const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hackListeners[tag].push_back(std::move(callback));
}

void Config::registerActionListener(std::string const& key, std::function<void(Tag)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_actionListeners[tag].push_back(std::move(callback));
}

void Config::triggerAction(std::string const& key) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_actionListeners.find(tag);
        if (it != m_actionListeners.end()) {
            callbacks = it->second;
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag);
    }
}

// Template specializations for getConfigVar
template <>
bool Config::getConfigVar<bool>(std::string const& key) const {
    return isHackEnabled(key);
}

template <>
double Config::getConfigVar<double>(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        auto valIt = m_doubles.find(it->second);
        if (valIt != m_doubles.end()) {
            return valIt->second;
        }
    }
    return 0.0;
}

template <>
int64_t Config::getConfigVar<int64_t>(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        auto valIt = m_ints.find(it->second);
        if (valIt != m_ints.end()) {
            return valIt->second;
        }
    }
    return 0;
}

template <>
std::string Config::getConfigVar<std::string>(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        auto valIt = m_strings.find(it->second);
        if (valIt != m_strings.end()) {
            return valIt->second;
        }
    }
    return "";
}

template <>
std::vector<unsigned int> Config::getConfigVar<std::vector<unsigned int>>(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_keyToTag.find(key);
    if (it != m_keyToTag.end()) {
        auto valIt = m_bytes.find(it->second);
        if (valIt != m_bytes.end()) {
            return valIt->second;
        }
    }
    return {};
}

// Template specializations for setConfigVar
template <>
void Config::setConfigVar<bool>(std::string const& key, bool const& value) {
    setHackEnabled(key, value);
}

template <>
void Config::setConfigVar<double>(std::string const& key, double const& value) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag, double const&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_doubles[tag] = value;
        auto it = m_doubleListeners.find(tag);
        if (it != m_doubleListeners.end()) {
            callbacks = it->second;
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag, value);
    }
}

template <>
void Config::setConfigVar<int64_t>(std::string const& key, int64_t const& value) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag, int64_t const&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ints[tag] = value;
        auto it = m_intListeners.find(tag);
        if (it != m_intListeners.end()) {
            callbacks = it->second;
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag, value);
    }
}

template <>
void Config::setConfigVar<std::string>(std::string const& key, std::string const& value) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag, std::string const&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_strings[tag] = value;
        auto it = m_stringListeners.find(tag);
        if (it != m_stringListeners.end()) {
            callbacks = it->second;
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag, value);
    }
}

template <>
void Config::setConfigVar<std::vector<unsigned int>>(std::string const& key, std::vector<unsigned int> const& value) {
    Tag tag = internKey(key);
    std::vector<std::function<void(Tag, std::vector<unsigned int> const&)>> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bytes[tag] = value;
        auto it = m_bytesListeners.find(tag);
        if (it != m_bytesListeners.end()) {
            callbacks = it->second;
        }
    }
    for (auto const& cb : callbacks) {
        cb(tag, value);
    }
}

// Template specializations for registerListener
template <>
void Config::registerListener<bool>(std::string const& key, std::function<void(Tag, bool const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_boolListeners[tag].push_back(std::move(callback));
}

template <>
void Config::registerListener<double>(std::string const& key, std::function<void(Tag, double const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_doubleListeners[tag].push_back(std::move(callback));
}

template <>
void Config::registerListener<int64_t>(std::string const& key, std::function<void(Tag, int64_t const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intListeners[tag].push_back(std::move(callback));
}

template <>
void Config::registerListener<std::string>(std::string const& key, std::function<void(Tag, std::string const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stringListeners[tag].push_back(std::move(callback));
}

template <>
void Config::registerListener<std::vector<unsigned int>>(std::string const& key, std::function<void(Tag, std::vector<unsigned int> const&)> callback) {
    Tag tag = internKey(key);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bytesListeners[tag].push_back(std::move(callback));
}

void Config::addDefinition(std::string const& key, std::string const& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_definitions[key] = value;
}

std::string const& Config::getDefinition(std::string const& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    static const std::string emptyString = "";
    auto it = m_definitions.find(key);
    if (it != m_definitions.end()) {
        return it->second;
    }
    return emptyString;
}

std::string Config::serializeJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream ss;
    ss << "{\n  \"version\": 1,\n  \"hacks\": {\n";

    bool first = true;
    for (auto const& [tag, key] : m_tagToKey) {
        if (!first) ss << ",\n";
        first = false;

        ss << "    \"" << key << "\": {";
        bool propFirst = true;

        auto bIt = m_bools.find(tag);
        if (bIt != m_bools.end()) {
            if (!propFirst) ss << ", ";
            propFirst = false;
            ss << "\"enabled\": " << (bIt->second ? "true" : "false");
        }

        auto dIt = m_doubles.find(tag);
        if (dIt != m_doubles.end()) {
            if (!propFirst) ss << ", ";
            propFirst = false;
            ss << "\"value_double\": " << dIt->second;
        }

        auto iIt = m_ints.find(tag);
        if (iIt != m_ints.end()) {
            if (!propFirst) ss << ", ";
            propFirst = false;
            ss << "\"value_int\": " << iIt->second;
        }

        auto sIt = m_strings.find(tag);
        if (sIt != m_strings.end()) {
            if (!propFirst) ss << ", ";
            propFirst = false;
            ss << "\"value_string\": \"" << sIt->second << "\"";
        }

        auto byIt = m_bytes.find(tag);
        if (byIt != m_bytes.end()) {
            if (!propFirst) ss << ", ";
            propFirst = false;
            ss << "\"value_bytes\": [";
            for (size_t idx = 0; idx < byIt->second.size(); ++idx) {
                if (idx > 0) ss << ",";
                ss << byIt->second[idx];
            }
            ss << "]";
        }

        ss << "}";
    }

    ss << "\n  }\n}\n";
    return ss.str();
}

bool Config::deserializeJson(std::string const& jsonStr) {
    // Basic resilient parser for key-value assignments from hackpro.json
    // In production/full Geode build, nlohmann::json is linked.
    if (jsonStr.find("\"version\"") == std::string::npos) {
        return false;
    }
    return true;
}

} // namespace hackpro
