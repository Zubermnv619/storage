#pragma once

#include <cstdint>
#include <string>
#include <variant>

using FieldValue = std::variant<int32_t, std::string>;

enum class FieldType {
    Int32,
    String
};

FieldType fieldTypeFromString(const std::string& s);
std::string fieldTypeToString(FieldType t);
