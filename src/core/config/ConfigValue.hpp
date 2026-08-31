#pragma once

// Atlas config: a full ConfigValue with defaults, and a mirror PartialConfig
// where every field is optional. The user's settings.jsonc deserializes into
// a PartialConfig; mergeWithUser() overlays only the fields the user set.
//
// Adapted from Vicinae's config pattern (src/server/src/config/config.hpp):
// a Partial<T> of all-optional fields merged over defaults, so the user file
// only ever needs to contain what it overrides.

#include <optional>

struct ConfigValue {
    int borderRounding = 12;
    int borderWidth = 1;
    double windowOpacity = 0.94;
    int windowWidth = 760;
    int windowHeight = 480;
};

struct PartialConfig {
    std::optional<int> borderRounding;
    std::optional<int> borderWidth;
    std::optional<double> windowOpacity;
    std::optional<int> windowWidth;
    std::optional<int> windowHeight;
};

inline ConfigValue mergeWithUser(const ConfigValue &defaults, const PartialConfig &user) {
    ConfigValue result = defaults;
    if (user.borderRounding) result.borderRounding = *user.borderRounding;
    if (user.borderWidth) result.borderWidth = *user.borderWidth;
    if (user.windowOpacity) result.windowOpacity = *user.windowOpacity;
    if (user.windowWidth) result.windowWidth = *user.windowWidth;
    if (user.windowHeight) result.windowHeight = *user.windowHeight;
    return result;
}
