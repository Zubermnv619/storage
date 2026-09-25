#!/usr/bin/env bash
set -euo pipefail
export PATH="/ucrt64/bin:/usr/bin:$PATH"
cd "$(dirname "$0")/.."
make clean
make all
echo "BUILD OK"
ls -l bin/
