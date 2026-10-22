
#include "hackpro/config.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cctype>
#include <map>
#include <vector>
#include <variant>

namespace hackpro {

Config::Config() = default;
Config::~Config() = default;

Config& Config::instance() {
    static Config s_instance;
    return s_instance;
}

Tag Config::internKey(std::string const& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return internKeyLocked(key);
}

Tag Config::internKeyLocked(std::string const& key) {
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

    std::vector<std::function<void(Tag)>> actionCallbacks;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto actionIt = m_actionListeners.find(tag);
        if (actionIt != m_actionListeners.end() && !actionIt->second.empty()) {
            actionCallbacks = actionIt->second;
        }
    }
    if (!actionCallbacks.empty()) {
        for (auto const& cb : actionCallbacks) {
            cb(tag);
        }
        return;
    }

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

namespace {

std::string escapeJson(std::string const& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\n"; break;
            case '\r': out += "\r"; break;
            case '\t': out += "\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

size_t findMatchingBrace(std::string const& s, size_t openPos) {
    int depth = 0;
    bool inStr = false;
    bool esc = false;
    for (size_t j = openPos; j < s.size(); ++j) {
        char c = s[j];
        if (inStr) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
        } else {
            if (c == '"') inStr = true;
            else if (c == '{') ++depth;
            else if (c == '}') {
                --depth;
                if (depth == 0) return j;
            }
        }
    }
    return std::string::npos;
}

void skipWs(std::string const& s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
}

} // namespace


std::string Config::serializeJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream ss;
    ss << "{\n  \"version\": 1,\n  \"hacks\": {\n";

    bool first = true;
    for (auto const& [tag, key] : m_tagToKey) {
        if (!first) ss << ",\n";
        first = false;

        ss << "    \"" << escapeJson(key) << "\": {";
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
            ss << "\"value_string\": \"" << escapeJson(sIt->second) << "\"";
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
    size_t hacksPos = jsonStr.find("\"hacks\"");
    if (jsonStr.find("\"version\"") == std::string::npos) return false;
    if (hacksPos == std::string::npos) return false;

    size_t objStart = jsonStr.find('{', hacksPos);
    if (objStart == std::string::npos) return false;
    size_t objEnd = findMatchingBrace(jsonStr, objStart);
    if (objEnd == std::string::npos) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_bools.clear();
    m_doubles.clear();
    m_ints.clear();
    m_strings.clear();
    m_bytes.clear();

    size_t i = objStart + 1;
    skipWs(jsonStr, i);
    if (i < jsonStr.size() && jsonStr[i] == '}') {
        return true;
    }

    while (i < objEnd) {
        skipWs(jsonStr, i);
        if (i >= objEnd || jsonStr[i] == '}') break;
        if (jsonStr[i] != '"') return false;
        std::string key;
        size_t kstart = ++i;
        while (i < jsonStr.size() && jsonStr[i] != '"') ++i;
        if (i >= jsonStr.size()) return false;
        key = jsonStr.substr(kstart, i - kstart);
        ++i; // closing quote
        skipWs(jsonStr, i);
        if (i >= jsonStr.size() || jsonStr[i] != ':') return false;
        ++i;
        skipWs(jsonStr, i);
        if (i >= jsonStr.size() || jsonStr[i] != '{') return false;
        size_t nestedStart = i;
        size_t nestedEnd = findMatchingBrace(jsonStr, nestedStart);
        if (nestedEnd == std::string::npos || nestedEnd > objEnd) return false;
        std::string nested = jsonStr.substr(nestedStart, nestedEnd - nestedStart + 1);

        Tag tag = internKeyLocked(key);

        auto getBool = [&](std::string const& field, bool& out) -> bool {
            size_t p = nested.find("\"" + field + "\"");
            if (p == std::string::npos) return false;
            size_t c = nested.find(':', p);
            if (c == std::string::npos) return false;
            size_t v = c + 1;
            skipWs(nested, v);
            if (nested.compare(v, 4, "true") == 0) { out = true; return true; }
            if (nested.compare(v, 5, "false") == 0) { out = false; return true; }
            return false;
        };
        auto getNumber = [&](std::string const& field, double& out) -> bool {
            size_t p = nested.find("\"" + field + "\"");
            if (p == std::string::npos) return false;
            size_t c = nested.find(':', p);
            if (c == std::string::npos) return false;
            size_t v = c + 1;
            skipWs(nested, v);
            size_t start = v;
            while (v < nested.size() && (std::isdigit(static_cast<unsigned char>(nested[v])) ||
                   nested[v] == '-' || nested[v] == '+' || nested[v] == '.' ||
                   nested[v] == 'e' || nested[v] == 'E')) ++v;
            if (v == start) return false;
            try { out = std::stod(nested.substr(start, v - start)); } catch (...) { return false; }
            return true;
        };
        auto getString = [&](std::string const& field, std::string& out) -> bool {
            size_t p = nested.find("\"" + field + "\"");
            if (p == std::string::npos) return false;
            size_t c = nested.find(':', p);
            if (c == std::string::npos) return false;
            size_t v = c + 1;
            skipWs(nested, v);
            if (v >= nested.size() || nested[v] != '"') return false;
            std::string val;
            size_t j = v + 1;
            while (j < nested.size() && nested[j] != '"') {
                if (nested[j] == '\\' && j + 1 < nested.size()) {
                    char e = nested[++j];
                    switch (e) {
                        case '"': val.push_back('"'); break;
                        case '\\': val.push_back('\\'); break;
                        case 'n': val.push_back('\n'); break;
                        case 'r': val.push_back('\r'); break;
                        case 't': val.push_back('\t'); break;
                        default: val.push_back(e); break;
                    }
                    ++j;
                } else {
                    val.push_back(nested[j++]);
                }
            }
            if (j >= nested.size()) return false;
            out = val;
            return true;
        };
        auto getBytes = [&](std::string const& field, std::vector<unsigned int>& out) -> bool {
            size_t p = nested.find("\"" + field + "\"");
            if (p == std::string::npos) return false;
            size_t c = nested.find(':', p);
            if (c == std::string::npos) return false;
            size_t v = c + 1;
            skipWs(nested, v);
            if (v >= nested.size() || nested[v] != '[') return false;
            size_t close = nested.find(']', v);
            if (close == std::string::npos) return false;
            std::string arr = nested.substr(v + 1, close - v - 1);
            std::vector<unsigned int> bytes;
            size_t pos2 = 0;
            while (pos2 < arr.size()) {
                while (pos2 < arr.size() && (arr[pos2] == ' ' || arr[pos2] == ',' || arr[pos2] == '\t' || arr[pos2] == '\n' || arr[pos2] == '\r')) ++pos2;
                if (pos2 >= arr.size()) break;
                size_t start = pos2;
                while (pos2 < arr.size() && (std::isdigit(static_cast<unsigned char>(arr[pos2])))) ++pos2;
                if (pos2 == start) break;
                try { bytes.push_back(static_cast<unsigned int>(std::stoul(arr.substr(start, pos2 - start)))); } catch (...) { break; }
            }
            out = std::move(bytes);
            return true;
        };

        bool bval = false;
        if (getBool("enabled", bval)) m_bools[tag] = bval;

        double dval = 0;
        if (getNumber("value_double", dval)) m_doubles[tag] = dval;

        double ival = 0;
        if (getNumber("value_int", ival)) m_ints[tag] = static_cast<int64_t>(ival);

        std::string sval;
        if (getString("value_string", sval)) m_strings[tag] = sval;

        std::vector<unsigned int> bytes;
        if (getBytes("value_bytes", bytes)) m_bytes[tag] = std::move(bytes);

        i = nestedEnd + 1;
        skipWs(jsonStr, i);
        if (i < objEnd && jsonStr[i] == ',') {
            ++i;
            continue;
        }
        break;
    }

    return true;
}

bool Config::loadFromFile(std::string const& path) {
    std::ifstream in(path);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    return deserializeJson(ss.str());
}

bool Config::saveToFile(std::string const& path) const {
    std::ofstream out(path);
    if (!out) return false;
    out << serializeJson();
    return static_cast<bool>(out);
}

} // namespace hackpro
