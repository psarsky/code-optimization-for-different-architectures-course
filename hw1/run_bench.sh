#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if [[ ! -x ./normalize0 || ! -x ./normalize1 || ! -x ./normalize2 || ! -x ./normalize3 || ! -x ./normalize4 || ! -x ./normalize5 || ! -x ./normalize6 ]]; then
  make -s
fi

python3 generate_test_input.py

INPUTS=(
  test_input_large.txt
  test_input_low_mod.txt
  test_input_mid_mod.txt
  test_input_high_mod.txt
)
VERSIONS=(normalize0 normalize1 normalize2 normalize3 normalize4 normalize5 normalize6)
REPEATS=5

echo "input,version,avg_ms,bytes_out" > bench_results.csv

for input in "${INPUTS[@]}"; do
  for v in "${VERSIONS[@]}"; do
    line="./$v $input output_${v}.txt $REPEATS"
    result=$($line)
    avg_ms=$(echo "$result" | sed -n 's/.*avg_ms=\([^ ]*\).*/\1/p')
    bytes_out=$(echo "$result" | sed -n 's/.*bytes_out=\([^ ]*\).*/\1/p')
    echo "$input,$v,$avg_ms,$bytes_out" >> bench_results.csv
  done
done

: > time_profile.txt
for v in "${VERSIONS[@]}"; do
  echo "[$v]" | tee -a time_profile.txt
  /usr/bin/time -f "elapsed_s=%e user_s=%U sys_s=%S cpu=%P maxrss_kb=%M" \
    "./$v" test_input_large.txt "output_${v}.txt" 1 \
    1>/dev/null 2>> time_profile.txt
  echo >> time_profile.txt
done
