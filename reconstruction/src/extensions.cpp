#include "hackpro/extensions.hpp"
#include "hackpro/config.hpp"
#include <iostream>

namespace hackpro {

std::string const& define(std::string const& key) {
    return Config::instance().getDefinition(key);
}

namespace extensions {

std::optional<hackpro::Tag> tagFor(std::string const& key) {
    return Config::instance().tagFor(key);
}

std::string keyFor(hackpro::Tag tag) {
    return Config::instance().keyFor(tag);
}

bool isHackEnabled(std::string const& key) {
    return Config::instance().isHackEnabled(key);
}

void setHackEnabled(std::string const& key, bool enabled) {
    Config::instance().setHackEnabled(key, enabled);
}

void registerHackListener(std::string const& key, std::function<void(Tag, bool const&)> callback) {
    Config::instance().registerHackListener(key, std::move(callback));
}

void registerActionListener(std::string const& key, std::function<void(Tag)> callback) {
    Config::instance().registerActionListener(key, std::move(callback));
}

bool registerTab(std::istream& stream, hackpro::extensions::Delegate* delegate) {
    if (!stream.good()) {
        return false;
    }
    // Tab registration parses JSON / gzip stream and associates delegate
    return true;
}

void showMessage(hackpro::MessageType type, std::string const& text, std::function<void()> onDismiss) {
    // In game UI this shows a popup modal. If dismissed, invokes onDismiss.
    if (onDismiss) {
        onDismiss();
    }
}

void showError(std::string const& message) {
    showMessage(hackpro::MessageType::Error, message, nullptr);
}

void addDefinition(std::string const& key, std::string const& definition) {
    Config::instance().addDefinition(key, definition);
}

// Specializations for getConfigVar
template <>
bool getConfigVar<bool>(std::string const& key) {
    return Config::instance().getConfigVar<bool>(key);
}

template <>
double getConfigVar<double>(std::string const& key) {
    return Config::instance().getConfigVar<double>(key);
}

template <>
int64_t getConfigVar<int64_t>(std::string const& key) {
    return Config::instance().getConfigVar<int64_t>(key);
}

template <>
std::string getConfigVar<std::string>(std::string const& key) {
    return Config::instance().getConfigVar<std::string>(key);
}

template <>
std::vector<unsigned int> getConfigVar<std::vector<unsigned int>>(std::string const& key) {
    return Config::instance().getConfigVar<std::vector<unsigned int>>(key);
}

// Specializations for setConfigVar
template <>
void setConfigVar<bool>(std::string const& key, bool const& value) {
    Config::instance().setConfigVar<bool>(key, value);
}

template <>
void setConfigVar<double>(std::string const& key, double const& value) {
    Config::instance().setConfigVar<double>(key, value);
}

template <>
void setConfigVar<int64_t>(std::string const& key, int64_t const& value) {
    Config::instance().setConfigVar<int64_t>(key, value);
}

template <>
void setConfigVar<std::string>(std::string const& key, std::string const& value) {
    Config::instance().setConfigVar<std::string>(key, value);
}

template <>
void setConfigVar<std::vector<unsigned int>>(std::string const& key, std::vector<unsigned int> const& value) {
    Config::instance().setConfigVar<std::vector<unsigned int>>(key, value);
}

// Specializations for registerListener
template <>
void registerListener<bool>(std::string const& key, std::function<void(Tag, bool const&)> callback) {
    Config::instance().registerListener<bool>(key, std::move(callback));
}

template <>
void registerListener<double>(std::string const& key, std::function<void(Tag, double const&)> callback) {
    Config::instance().registerListener<double>(key, std::move(callback));
}

template <>
void registerListener<int64_t>(std::string const& key, std::function<void(Tag, int64_t const&)> callback) {
    Config::instance().registerListener<int64_t>(key, std::move(callback));
}

template <>
void registerListener<std::string>(std::string const& key, std::function<void(Tag, std::string const&)> callback) {
    Config::instance().registerListener<std::string>(key, std::move(callback));
}

template <>
void registerListener<std::vector<unsigned int>>(std::string const& key, std::function<void(Tag, std::vector<unsigned int> const&)> callback) {
    Config::instance().registerListener<std::vector<unsigned int>>(key, std::move(callback));
}

} // namespace extensions
} // namespace hackpro

namespace mega::utils::safe_mode {

static bool s_cheatsEnabled = true;

bool areCheatsEnabled() {
    return s_cheatsEnabled;
}

void setCheatsEnabled(bool enabled) {
    s_cheatsEnabled = enabled;
}

} // namespace mega::utils::safe_mode
