#include "record.hpp"

#include <sstream>
#include <stdexcept>

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

    std::stringstream ss(line);
    std::string token;
    size_t fieldIdx = 0;

    while (std::getline(ss, token, '|')) {
        if (fieldIdx >= schema.fieldCount()) {
            throw std::runtime_error("Data line has more fields than schema: " + line);
        }
        const FieldDef& def = schema.fields()[fieldIdx];
        if (def.type == FieldType::Int32) {
            rec.values.emplace_back(static_cast<int32_t>(std::stol(token)));
        } else {
            rec.values.emplace_back(token);
        }
        ++fieldIdx;
    }

    if (fieldIdx != schema.fieldCount()) {
        throw std::runtime_error("Data line has fewer fields than schema: " + line);
    }
    return rec;
}

std::string serializeRecord(const Record& rec) {
    std::ostringstream out;
    for (size_t i = 0; i < rec.values.size(); ++i) {
        if (i > 0) out << '|';
        const FieldValue& v = rec.values[i];
        if (std::holds_alternative<int32_t>(v)) {
            out << std::get<int32_t>(v);
        } else {
            out << std::get<std::string>(v);
        }
    }
    return out.str();
}
