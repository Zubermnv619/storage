#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

# MSYS installations may keep the selected MinGW toolchain outside PATH.
if ! command -v g++ >/dev/null 2>&1 && [ -x /ucrt64/bin/g++.exe ]; then
	export PATH="/ucrt64/bin:$PATH"
	export CXX=/ucrt64/bin/g++.exe
fi

make clean
make all

echo ""
echo "Build complete:"
echo "  bin/kv_loader       - the distributed KV loader"
echo "  bin/generate_data   - test data generator"
