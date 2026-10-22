#pragma once

#include "types.hpp"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <iostream>

namespace hackpro {

class HACKPRO_API BasicConfig {
public:
    virtual ~BasicConfig() = default;

    virtual std::optional<Tag> tagFor(std::string const& key) const = 0;
    virtual std::string keyFor(Tag tag) const = 0;

    virtual bool isHackEnabled(std::string const& key) const = 0;
    virtual void setHackEnabled(std::string const& key, bool enabled) = 0;
};

class HACKPRO_API Config : public BasicConfig {
public:
    Config();
    ~Config() override;

    static Config& instance();

    // Key / Tag mapping
    std::optional<Tag> tagFor(std::string const& key) const override;
    std::string keyFor(Tag tag) const override;
    Tag internKey(std::string const& key);

    // Hack enable toggles
    bool isHackEnabled(std::string const& key) const override;
    void setHackEnabled(std::string const& key, bool enabled) override;
    void registerHackListener(std::string const& key, std::function<void(Tag, bool const&)> callback);

    // Action listeners (e.g. buttons)
    void registerActionListener(std::string const& key, std::function<void(Tag)> callback);
    void triggerAction(std::string const& key);

    // Typed configuration getters/setters/listeners
    template <typename T>
    T getConfigVar(std::string const& key) const;

    template <typename T>
    void setConfigVar(std::string const& key, T const& value);

    template <typename T>
    void registerListener(std::string const& key, std::function<void(Tag, T const&)> callback);

    // Definitions
    void addDefinition(std::string const& key, std::string const& value);
    std::string const& getDefinition(std::string const& key) const;

    // Serialization
    std::string serializeJson() const;
    bool deserializeJson(std::string const& jsonStr);
    bool loadFromFile(std::string const& path);
    bool saveToFile(std::string const& path) const;

private:
    mutable std::mutex m_mutex;
    Tag internKeyLocked(std::string const& key);

    // Bi-directional key <-> tag interning
    std::unordered_map<std::string, Tag> m_keyToTag;
    std::unordered_map<Tag, std::string> m_tagToKey;
    Tag m_nextTag{1};

    // Typed value stores
    std::unordered_map<Tag, bool> m_bools;
    std::unordered_map<Tag, double> m_doubles;
    std::unordered_map<Tag, int64_t> m_ints;
    std::unordered_map<Tag, std::string> m_strings;
    std::unordered_map<Tag, std::vector<unsigned int>> m_bytes;

    // Typed listener callbacks
    std::unordered_map<Tag, std::vector<std::function<void(Tag, bool const&)>>> m_boolListeners;
    std::unordered_map<Tag, std::vector<std::function<void(Tag, double const&)>>> m_doubleListeners;
    std::unordered_map<Tag, std::vector<std::function<void(Tag, int64_t const&)>>> m_intListeners;
    std::unordered_map<Tag, std::vector<std::function<void(Tag, std::string const&)>>> m_stringListeners;
    std::unordered_map<Tag, std::vector<std::function<void(Tag, std::vector<unsigned int> const&)>>> m_bytesListeners;

    std::unordered_map<Tag, std::vector<std::function<void(Tag, bool const&)>>> m_hackListeners;
    std::unordered_map<Tag, std::vector<std::function<void(Tag)>>> m_actionListeners;

    // UI definitions
    std::unordered_map<std::string, std::string> m_definitions;
};

} // namespace hackpro
