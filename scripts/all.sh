#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
CONFIG="${1:-config/cluster.conf}"

./scripts/build.sh
./scripts/generate_data.sh "$CONFIG"
./scripts/run.sh "$CONFIG"
