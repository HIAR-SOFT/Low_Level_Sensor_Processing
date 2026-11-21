#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

RUNS="${RUNS:-30}"
DATA="${DATA:-data/generated/vibration_mixed_1000000.bin}"
[[ -f "$DATA" ]] || { echo "Missing $DATA -- run: make data" >&2; exit 1; }

make -s all

STAMP="$(date +%Y%m%d_%H%M%S)"
CSV="results/raw/bench_${STAMP}.csv"
MEM="results/raw/memory_${STAMP}.csv"
ENVF="results/raw/environment_${STAMP}.txt"
mkdir -p results/raw

{
  date; uname -a
  lscpu | grep -E 'Model name|^CPU\(s\)|Thread|L1d|L2|L3|Hypervisor'
  gcc --version | head -1; nasm -v
  echo "C flags: -std=c11 -g -ffp-contract=off (O0/O2/O3 per binary)"
  echo "RUNS=$RUNS DATA=$DATA CPU_PIN=${CPU:-none} THRESHOLD=${THRESHOLD:-2.5}"
} > "$ENVF"

PIN=()
[[ -n "${CPU:-}" ]] && PIN=(taskset -c "$CPU")

echo "impl,algorithm,n,run,ns_per_call,tsc_per_call,iters" > "$CSV"
echo "impl,n,vmhwm_kb" > "$MEM"

for n in 1000 10000 100000 1000000; do
  for cfg in "bench_o0:c:c_O0" "bench_o2:c:c_O2" "bench_o3:c:c_O3" "bench_o2:asm:asm"; do
    IFS=: read -r bin impl label <<< "$cfg"
    echo "n=$n  $label" >&2
    "${PIN[@]}" "./build/$bin" --impl "$impl" --label "$label" \
        --data "$DATA" --n "$n" --runs "$RUNS" --threshold "${THRESHOLD:-2.5}" >> "$CSV" 2>> "$MEM"
  done
done
echo "Wrote $CSV"