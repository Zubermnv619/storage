#include "cluster_config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

constexpr char kWhitespace[] = " \t\r\n";

// Converts num_nodes, rejecting both non-numeric input and trailing junk.
// std::stoi itself throws std::invalid_argument / std::out_of_range; those are
// re-thrown as std::runtime_error with the offending value so the operator
// sees something actionable instead of "stoi".
int parseNodeCount(const std::string& value) {
    std::size_t consumed = 0;
    int parsed = 0;
    try {
        parsed = std::stoi(value, &consumed);
    } catch (const std::exception& ex) {
        throw std::runtime_error("num_nodes is not a valid integer ('" + value + "'): " + ex.what());
    }
    if (consumed != value.size()) {
        throw std::runtime_error("num_nodes has trailing characters: '" + value + "'");
    }
    return parsed;
}

}  // namespace

std::string trim(const std::string& s) {
    const std::size_t start = s.find_first_not_of(kWhitespace);
    if (start == std::string::npos) return "";
    const std::size_t end = s.find_last_not_of(kWhitespace);
    return s.substr(start, end - start + 1);
}

ClusterConfig loadClusterConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open cluster config: " + path);
    }

    ClusterConfig cfg;
    std::string line;
    std::size_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        const std::size_t eq = trimmed.find('=');
        if (eq == std::string::npos) {
            throw std::runtime_error("Malformed config line " + std::to_string(lineNo)
                                     + " (expected key=value): " + line);
        }
        const std::string key = trim(trimmed.substr(0, eq));
        const std::string val = trim(trimmed.substr(eq + 1));

        if (key == "num_nodes") {
            cfg.numNodes = parseNodeCount(val);
        } else if (key == "data_dir") {
            cfg.dataDir = val;
        } else if (key == "field") {
            const std::size_t colon = val.find(':');
            if (colon == std::string::npos) {
                throw std::runtime_error("Malformed field entry on line "
                                         + std::to_string(lineNo)
                                         + " (expected name:type): " + val);
            }
            const std::string fname = trim(val.substr(0, colon));
            const std::string ftype = trim(val.substr(colon + 1));
            cfg.schema.addField(fname, fieldTypeFromString(ftype));
        } else {
            throw std::runtime_error("Unknown config key '" + key + "' on line "
                                     + std::to_string(lineNo));
        }
    }

    if (cfg.numNodes < 1 || cfg.numNodes > 5) {
        throw std::runtime_error("num_nodes must be between 1 and 5 (mock cluster limit), got "
                                 + std::to_string(cfg.numNodes));
    }
    if (cfg.schema.fieldCount() == 0) {
        throw std::runtime_error("Schema has no fields; add at least one 'field=' entry");
    }
    return cfg;
}

std::string dataFilePath(const ClusterConfig& cfg, int nodeId) {
    std::ostringstream oss;
    oss << cfg.dataDir << "/node" << nodeId << ".dat";
    return oss.str();
}
