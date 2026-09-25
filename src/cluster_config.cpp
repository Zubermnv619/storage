#include "cluster_config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

ClusterConfig loadClusterConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open cluster config: " + path);
    }

    ClusterConfig cfg;
    std::string line;
    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) {
            throw std::runtime_error("Malformed config line (expected key=value): " + line);
        }
        std::string key = trim(trimmed.substr(0, eq));
        std::string val = trim(trimmed.substr(eq + 1));

        if (key == "num_nodes") {
            cfg.numNodes = std::stoi(val);
        } else if (key == "data_dir") {
            cfg.dataDir = val;
        } else if (key == "field") {
            size_t colon = val.find(':');
            if (colon == std::string::npos) {
                throw std::runtime_error("Malformed field entry (expected name:type): " + val);
            }
            std::string fname = trim(val.substr(0, colon));
            std::string ftype = trim(val.substr(colon + 1));
            cfg.schema.addField(fname, fieldTypeFromString(ftype));
        }
    }

    if (cfg.numNodes < 1 || cfg.numNodes > 5) {
        throw std::runtime_error("num_nodes must be between 1 and 5 (mock cluster limit)");
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
