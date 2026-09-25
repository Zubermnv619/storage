#pragma once

#include <cstdint>
#include <string>
#include <variant>

// A single field value. The config schema only exposes int32 and string, so
// the variant is closed over exactly those two alternatives.
using FieldValue = std::variant<int32_t, std::string>;

enum class FieldType {
    Int32,
    String
};

// Throws std::invalid_argument for an unrecognised type name.
[[nodiscard]] FieldType fieldTypeFromString(const std::string& s);

[[nodiscard]] std::string fieldTypeToString(FieldType t);
