#pragma once

#include <string>
#include <vector>

#include "schema.hpp"
#include "types.hpp"

// One parsed data row. `values` is positional and matches the declaring
// Schema's field order; values.front() is the record key.
struct Record {
    std::vector<FieldValue> values;

    [[nodiscard]] const FieldValue& key() const;
    [[nodiscard]] std::string keyAsString() const;
};

// Parses a single '|'-delimited line against `schema`.
//
// Throws std::runtime_error when the field count does not match the schema, or
// when an int32 field is not a valid integer (std::invalid_argument /
// std::out_of_range propagate from std::stol). Callers in the load loop catch
// these and skip the line rather than aborting the whole run.
[[nodiscard]] Record parseRecordLine(const std::string& line, const Schema& schema);

// Re-serialises a Record in the same '|'-delimited format, so it can be sent
// over the mock transport and parsed again on the receiving node.
[[nodiscard]] std::string serializeRecord(const Record& rec);
