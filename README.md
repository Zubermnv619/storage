# Distributed In-Memory KV Storage & Loader

A C++17 program that simulates a 1-5 node distributed key-value
cluster, loads records from per-node data files, routes each record to
the single node that owns it (no replication), and reports load
statistics plus an ownership verification pass.

## Architecture

The implementation is modular: `Schema`, `Record`, and `ClusterConfig`
handle data definitions and parsing; `Partitioner` computes ownership;
`KvStore` provides a thread-safe per-node map; `MockInbox` and
`NetworkHub` provide `connect`-style mock transport; and `Node` loads,
batches, routes, and receives records. `main.cpp` starts the cluster and
performs the verification pass.

Ownership is `FNV-1a(key) % node_count`. A node stores local records it owns
and appends other records to a per-destination batch. Batches travel through
the destination inbox and are parsed by its receiver thread.

```
node A loader --send(batch)--> NetworkHub --> node B inbox --recv--> node B store
```

## Requirements

- A C++17 compiler 
- GNU Make

## Layout

```
config/cluster.conf     Cluster + schema configuration
include/                Public headers only
src/                    All implementation files, including main and generator
scripts/                Thin shell wrappers around `make`
data/                   Generated data files land here (git-ignored)
```

## Quick start

```bash
git clone <this repo>
cd dht_project
./scripts/run.sh all      # build, generate >=100MB/node test data, run
```

That builds `kv_loader` and `generate_data`, generates one ~100MB file per node under `data`, and runs the load job against
`config/cluster.conf`.

## Step by step

### 1. Build

```bash
./scripts/build.sh
```

Produces `kv_loader` and `generate_data`.

### 2. Configure the cluster

Edit `config/cluster.conf`:

```ini
num_nodes=4             
data_dir=data

field=user_id:int32
field=user_name:string
field=country:string
field=score:int32
```

### 3. Generate test data

```bash
./scripts/generate_data.sh                       # config/cluster.conf, 100MB/node
./scripts/generate_data.sh config/cluster.conf 209715200 0.25   # 200MB/node, 25% duplicate keys
```

This writes `data/node0.dat` ... `data/node<N-1>.dat`, each a plain text
file with one `|`-delimited record per line, matching the schema field
order. 

### 4. Run the load job

```bash
./scripts/run.sh                       
./scripts/run.sh path/to/other.conf
./bin/kv_loader config/cluster.conf --verify-sample=5
```

Sample output:

```
Loaded config: 4 node(s), 4 field(s), key field='user_id'

== Starting cluster ==

== Load statistics ==
  node0: linesRead=3123456 storedRecords=1980001 (inserted=1980001, overwritten=1143455, fromNetwork=2360004)
  node1: linesRead=3121980 storedRecords=1978890 (inserted=1978890, overwritten=1143090, fromNetwork=2358112)
  ...
  ---
  total unique keys stored across cluster: 7912345
  elapsed: 4.812s

== Ownership verification (all keys) ==
  key=650149 storedOn=node0 expectedOwner=node0  [OK]
  ...
All sampled keys are on their correct owner node.
```

- **`linesRead`** — records this node read from its own local file.
- **`storedRecords`** — unique keys this node currently owns and holds.
- **`inserted` / `overwritten`** — first-time vs. duplicate-key stores
  (duplicates are expected; last write wins).
- **`fromNetwork`** — of `storedRecords`, how many arrived from a peer
  rather than being already-local.
- **Ownership verification** re-derives every stored key's expected owner
  from the partitioner independently of the routing code. Use
  `--verify-sample=N` for a bounded check on very large runs. This is the
  "evidence records are stored on their corresponding owned nodes" the
  assignment asks for — it's a correctness check, not just a trust-me
  log line.

Exit code is `0` if verification passes, `1` otherwise.

### 5. Test and clean up

```bash
make test
./scripts/test.sh
```

The test target covers parsing, storage, partitioning, mock transport, and
an in-process three-node cluster. The shell smoke test generates a small
dataset, runs the loader, checks its exit status, and requires the ownership
success message.

```bash
make clean     # removes bin/ and generated data
```

## Running with your own data

You can run with own data just drop any `data/node<ID>.dat`
files in yourself, as long as each line is `|`-delimited and matches the
field order and types in `config/cluster.conf`. Malformed lines are
logged and skipped rather than aborting the whole load.

## Duplicate handling

The generator's third argument is `duplicateRatio`; for example,
`./scripts/generate_data.sh config/cluster.conf 1048576 0.25` creates a
small dataset with approximately 25% reused keys. `KvStore` keeps the last
value for duplicate keys and reports inserted versus overwritten records.

## Scalability & Fault Assumptions

Changing the node count causes a full remap because ownership is currently
`hash(key) % N`; records would need to be reloaded or redistributed. At
roughly 50-500 nodes, consistent hashing with virtual nodes would move fewer
keys during membership changes and smooth uneven hash ranges. This project
assumes no node-failure recovery, uses a single-process mock transport, and
does not persist records to disk.
