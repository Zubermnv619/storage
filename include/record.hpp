#pragma once

#include <string>
#include <vector>

#include "schema.hpp"
#include "types.hpp"

struct Record {
    std::vector<FieldValue> values;

    const FieldValue& key() const;
    std::string keyAsString() const;
};

Record parseRecordLine(const std::string& line, const Schema& schema);
std::string serializeRecord(const Record& rec);
