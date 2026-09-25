set -euo pipefail

cd "$(dirname "$0")/.."
make clean
make all

echo ""
echo "Build complete:"
echo "  bin/kv_loader       - the distributed KV loader"
echo "  bin/generate_data   - test data generator"
