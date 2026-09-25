#pragma once

#include <string>

#include "schema.hpp"

struct ClusterConfig {
    int numNodes = 1;
    std::string dataDir = "data";
    Schema schema;
};

std::string trim(const std::string& s);
ClusterConfig loadClusterConfig(const std::string& path);
std::string dataFilePath(const ClusterConfig& cfg, int nodeId);
