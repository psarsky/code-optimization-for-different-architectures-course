#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

make -s

SIZES=(256 512 1024 2048)
VERSIONS=(gauss0 gauss1 gauss2)
REPEATS=5

echo "n,version,avg_ms,best_ms,gflops_avg,gflops_best,max_diff,residual,status" > bench_results.csv

for n in "${SIZES[@]}"; do
  for v in "${VERSIONS[@]}"; do
    out=$("./$v" "$n" "$REPEATS")
    echo "$out"
    avg=$(sed -n 's/.*avg_ms=\([^ ]*\).*/\1/p'        <<<"$out")
    best=$(sed -n 's/.*best_ms=\([^ ]*\).*/\1/p'      <<<"$out")
    ga=$(sed -n 's/.*gflops_avg=\([^ ]*\).*/\1/p'     <<<"$out")
    gb=$(sed -n 's/.*gflops_best=\([^ ]*\).*/\1/p'    <<<"$out")
    diff=$(sed -n 's/.*max_diff=\([^ ]*\).*/\1/p'     <<<"$out")
    res=$(sed -n 's/.*residual=\([^ ]*\).*/\1/p'      <<<"$out")
    st=$(sed -n 's/.*status=\([^ ]*\).*/\1/p'         <<<"$out")
    echo "$n,$v,$avg,$best,$ga,$gb,$diff,$res,$st" >> bench_results.csv
  done
done

echo
echo "Wyniki zapisane w bench_results.csv"
