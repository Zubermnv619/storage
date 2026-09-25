#include "types.hpp"

#include <stdexcept>

FieldType fieldTypeFromString(const std::string& s) {
    if (s == "int32") return FieldType::Int32;
    if (s == "string") return FieldType::String;
    throw std::invalid_argument("Unknown field type in config: " + s);
}

std::string fieldTypeToString(FieldType t) {
    switch (t) {
        case FieldType::Int32: return "int32";
        case FieldType::String: return "string";
    }
    return "unknown";
}
