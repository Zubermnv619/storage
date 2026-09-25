#!/usr/bin/env bash
# Scratch diagnostic: confirm the toolchain the README assumes is reachable.
cd "$(dirname "$0")/.."
echo "pwd=$(pwd)"
export PATH="/ucrt64/bin:/usr/bin:$PATH"
for t in make g++ curl bash; do
    printf '%-6s: ' "$t"
    command -v "$t" || echo MISSING
done
g++ --version | head -1
make --version | head -1
curl --version | head -1
