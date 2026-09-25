set -euo pipefail

cd "$(dirname "$0")/.."

CONFIG="${1:-config/cluster.conf}"
TARGET_BYTES="${2:-104857600}"   # 100 MiB
DUP_RATIO="${3:-0.15}"

if [ ! -x bin/generate_data ]; then
    echo "bin/generate_data not found, building first!!!!"
    ./scripts/build.sh
fi

mkdir -p data
./bin/generate_data "$CONFIG" "$TARGET_BYTES" "$DUP_RATIO"
