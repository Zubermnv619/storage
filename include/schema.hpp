#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "types.hpp"

struct FieldDef {
    std::string name;
    FieldType type;
};

class Schema {
public:
    void addField(const std::string& name, FieldType type);
    const std::vector<FieldDef>& fields() const;
    size_t fieldCount() const;
    const FieldDef& keyField() const;

private:
    std::vector<FieldDef> fields_;
};
