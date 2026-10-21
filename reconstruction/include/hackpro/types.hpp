#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <variant>

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(HACKPRO_BUILD_DLL)
        #define HACKPRO_API __declspec(dllexport)
    #elif defined(HACKPRO_USE_DLL)
        #define HACKPRO_API __declspec(dllimport)
    #else
        #define HACKPRO_API
    #endif
#else
    #define HACKPRO_API __attribute__((visibility("default")))
#endif

namespace hackpro {

using Tag = uint64_t;
inline constexpr Tag INVALID_TAG = 0;

enum class MessageType : int {
    Info = 0,
    Warning = 1,
    Error = 2
};

enum class ValueType : uint8_t {
    Bool = 0,
    Decimal = 1,
    Int = 2,
    String = 3,
    Bytes = 4
};

using ConfigValue = std::variant<bool, double, int64_t, std::string, std::vector<unsigned int>>;

namespace extensions {

class Delegate {
public:
    virtual ~Delegate() = default;
};

} // namespace extensions
} // namespace hackpro
