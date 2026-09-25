#include "schema.hpp"

#include <stdexcept>

void Schema::addField(const std::string& name, FieldType type) {
    fields_.push_back({name, type});
}

const std::vector<FieldDef>& Schema::fields() const {
    return fields_;
}

size_t Schema::fieldCount() const {
    return fields_.size();
}

const FieldDef& Schema::keyField() const {
    if (fields_.empty()) {
        throw std::runtime_error("Schema has no fields, cannot determine key field");
    }
    return fields_.front();
}
