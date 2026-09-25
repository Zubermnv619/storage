#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "types.hpp"

struct FieldDef {
    std::string name;
    FieldType type;
};

// Ordered list of record fields. The FIRST declared field is the record key
// used for partitioning, matching the config file's declaration order.
class Schema {
public:
    void addField(const std::string& name, FieldType type);

    [[nodiscard]] const std::vector<FieldDef>& fields() const;
    [[nodiscard]] std::size_t fieldCount() const;

    // Throws std::runtime_error when the schema is empty.
    [[nodiscard]] const FieldDef& keyField() const;

private:
    std::vector<FieldDef> fields_;
};
