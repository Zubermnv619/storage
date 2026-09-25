#pragma once

#include <string>

#include "schema.hpp"

// Parsed contents of config/cluster.conf: cluster size, on-disk data
// directory, and the record schema (whose first field is the key).
struct ClusterConfig {
    int numNodes = 1;
    std::string dataDir = "data";
    Schema schema;
};

// Strips leading/trailing ASCII whitespace. Exposed because the config parser
// and its tests both rely on identical trimming rules.
[[nodiscard]] std::string trim(const std::string& s);

// Reads and validates a key=value config file.
//
// Throws std::runtime_error for a missing file, a malformed line, an
// out-of-range num_nodes, or an empty schema; std::invalid_argument /
// std::out_of_range can propagate from the integer conversion of num_nodes.
[[nodiscard]] ClusterConfig loadClusterConfig(const std::string& path);

// Path of a node's data file, e.g. "data/node2.dat".
[[nodiscard]] std::string dataFilePath(const ClusterConfig& cfg, int nodeId);
