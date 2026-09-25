set -euo pipefail

cd "$(dirname "$0")/.."

CONFIG="${1:-config/cluster.conf}"

if [ ! -x bin/kv_loader ]; then
    echo "bin/kv_loader not found, building first!!"
    ./scripts/build.sh
fi

./bin/kv_loader "$CONFIG"
