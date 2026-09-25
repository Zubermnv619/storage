#include "record.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace {

constexpr char kDelimiter = '|';

// Converts one token to the FieldDef's type, wrapping conversion failures with
// the field name and offending text so a skipped line is self-explanatory.
FieldValue parseField(const std::string& token, const FieldDef& def) {
    if (def.type != FieldType::Int32) {
        return token;
    }
    std::size_t consumed = 0;
    long parsed = 0;
    try {
        parsed = std::stol(token, &consumed, 10);
    } catch (const std::exception& ex) {
        throw std::runtime_error("field '" + def.name + "' is not a valid int32 ('"
                                 + token + "'): " + ex.what());
    }
    if (consumed != token.size()) {
        throw std::runtime_error("field '" + def.name + "' has trailing characters ('"
                                 + token + "')");
    }
    return static_cast<int32_t>(parsed);
}

}  // namespace

const FieldValue& Record::key() const {
    if (values.empty()) {
        throw std::runtime_error("Record has no fields");
    }
    return values.front();
}

std::string Record::keyAsString() const {
    const FieldValue& k = key();
    if (std::holds_alternative<int32_t>(k)) {
        return std::to_string(std::get<int32_t>(k));
    }
    return std::get<std::string>(k);
}

Record parseRecordLine(const std::string& line, const Schema& schema) {
    Record rec;
    rec.values.reserve(schema.fieldCount());

    std::size_t start = 0;
    for (;;) {
        if (rec.values.size() >= schema.fieldCount()) {
            throw std::runtime_error("Data line has more fields than schema: " + line);
        }
        const std::size_t bar = line.find(kDelimiter, start);
        const std::string token =
            (bar == std::string::npos) ? line.substr(start) : line.substr(start, bar - start);
        rec.values.push_back(parseField(token, schema.fields()[rec.values.size()]));
        if (bar == std::string::npos) break;
        start = bar + 1;
    }

    if (rec.values.size() != schema.fieldCount()) {
        throw std::runtime_error("Data line has fewer fields than schema: " + line);
    }
    return rec;
}

std::string serializeRecord(const Record& rec) {
    std::string out;
    for (std::size_t i = 0; i < rec.values.size(); ++i) {
        if (i > 0) out.push_back(kDelimiter);
        const FieldValue& v = rec.values[i];
        if (std::holds_alternative<int32_t>(v)) {
            out += std::to_string(std::get<int32_t>(v));
        } else {
            out += std::get<std::string>(v);
        }
    }
    return out;
}
