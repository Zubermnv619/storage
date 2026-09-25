#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make all
./bin/generate_data config/cluster.conf 8192 0.25
output="$(./bin/kv_loader config/cluster.conf)"
printf '%s\n' "$output"
printf '%s\n' "$output" | grep -F "All sampled keys are on their correct owner node."
