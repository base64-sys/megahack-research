#pragma once

#include "types.hpp"
#include <iostream>

namespace hackpro {

HACKPRO_API std::string const& define(std::string const& key);

namespace extensions {

// Key / Tag identity
HACKPRO_API std::optional<hackpro::Tag> tagFor(std::string const& key);
HACKPRO_API std::string keyFor(hackpro::Tag tag);

// Hack enable state
HACKPRO_API bool isHackEnabled(std::string const& key);
HACKPRO_API void setHackEnabled(std::string const& key, bool enabled);
HACKPRO_API void registerHackListener(std::string const& key, std::function<void(Tag, bool const&)> callback);

// Action listeners
HACKPRO_API void registerActionListener(std::string const& key, std::function<void(Tag)> callback);

// UI and definitions
HACKPRO_API bool registerTab(std::istream& stream, hackpro::extensions::Delegate* delegate);
HACKPRO_API void showMessage(hackpro::MessageType type, std::string const& text, std::function<void()> onDismiss);
HACKPRO_API void showError(std::string const& message);
HACKPRO_API void addDefinition(std::string const& key, std::string const& definition);

// Config variables (explicit template instantiations)
template <typename T>
HACKPRO_API T getConfigVar(std::string const& key);

template <typename T>
HACKPRO_API void setConfigVar(std::string const& key, T const& value);

template <typename T>
HACKPRO_API void registerListener(std::string const& key, std::function<void(Tag, T const&)> callback);

// Explicit declarations for the 5 exported types
template <> HACKPRO_API bool getConfigVar<bool>(std::string const& key);
template <> HACKPRO_API double getConfigVar<double>(std::string const& key);
template <> HACKPRO_API int64_t getConfigVar<int64_t>(std::string const& key);
template <> HACKPRO_API std::string getConfigVar<std::string>(std::string const& key);
template <> HACKPRO_API std::vector<unsigned int> getConfigVar<std::vector<unsigned int>>(std::string const& key);

template <> HACKPRO_API void setConfigVar<bool>(std::string const& key, bool const& value);
template <> HACKPRO_API void setConfigVar<double>(std::string const& key, double const& value);
template <> HACKPRO_API void setConfigVar<int64_t>(std::string const& key, int64_t const& value);
template <> HACKPRO_API void setConfigVar<std::string>(std::string const& key, std::string const& value);
template <> HACKPRO_API void setConfigVar<std::vector<unsigned int>>(std::string const& key, std::vector<unsigned int> const& value);

template <> HACKPRO_API void registerListener<bool>(std::string const& key, std::function<void(Tag, bool const&)> callback);
template <> HACKPRO_API void registerListener<double>(std::string const& key, std::function<void(Tag, double const&)> callback);
template <> HACKPRO_API void registerListener<int64_t>(std::string const& key, std::function<void(Tag, int64_t const&)> callback);
template <> HACKPRO_API void registerListener<std::string>(std::string const& key, std::function<void(Tag, std::string const&)> callback);
template <> HACKPRO_API void registerListener<std::vector<unsigned int>>(std::string const& key, std::function<void(Tag, std::vector<unsigned int> const&)> callback);

} // namespace extensions
} // namespace hackpro

namespace mega::utils::safe_mode {

HACKPRO_API bool areCheatsEnabled();
HACKPRO_API void setCheatsEnabled(bool enabled);

} // namespace mega::utils::safe_mode
