set -euo pipefail

cd "$(dirname "$0")/.."

if [ "${1:-}" = "all" ]; then
    CONFIG="${2:-config/cluster.conf}"
    ./scripts/build.sh
    ./scripts/generate_data.sh "$CONFIG"
else
    CONFIG="${1:-config/cluster.conf}"
fi

if [ ! -x bin/kv_loader ]; then
    echo "bin/kv_loader not found, building first!!"
    ./scripts/build.sh
fi

./bin/kv_loader "$CONFIG"
