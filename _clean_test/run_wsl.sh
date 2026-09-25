#!/usr/bin/env bash
# Reproduction driver: run the README quick start on a clean clone inside WSL.
set -u

CLONE_DIR="${1:-$HOME/clean_test/clone1}"
cd "$CLONE_DIR" || { echo "clone not found: $CLONE_DIR"; exit 2; }

echo "=== git modes (scripts) ==="
ls -l scripts/
echo
echo "=== file(1) ==="
file scripts/run.sh
echo
echo "=== README step: ./scripts/run.sh all ==="
./scripts/run.sh all
rc=$?
echo "EXIT=$rc"
exit $rc
