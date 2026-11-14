#!/usr/bin/env bash
#
# Run stage 2 of the OpenCV pipeline (template-matching) over every
# experiment listed in the input table.
#
# Stage 2 takes no arguments: it prompts for six values and reads them
# from stdin. This driver feeds it one table row at a time, so each
# experiment runs in its own process with its own inputs -- the batch
# does not depend on the program looping internally.
#
# Rows are processed in order and failures are counted, not fatal: one
# bad experiment (missing marker templates, unreadable sequence) should
# not abandon the other sixty-nine.

set -uo pipefail

usage () {
    cat <<'USAGE'
Usage: 04_run_template_matching.sh <template-matching-binary> [input_table.txt]

  <template-matching-binary>  the stage 2 executable built from the OpenCV project
  [input_table.txt]           default: ../work/stage2_input_table.txt (written by step 3)

Each row of the table must hold six tab-separated fields, in the order
stage 2 reads them:

  experiment_folder  experiment_name  initial_length  motor_frequency  fps  pixels_per_mm

Example:
  04_run_template_matching.sh ../../opencv/build/template-matching
USAGE
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage >&2
    exit 1
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
binary=$1
input_table=${2:-$script_dir/../work/stage2_input_table.txt}

[[ -x $binary ]] || { echo "error: not an executable: $binary" >&2; exit 1; }
[[ -f $input_table ]] || { echo "error: no such file: $input_table" >&2; exit 1; }

total=0
failed=0

while IFS=$'\t' read -r folder name length frequency fps pixels_per_mm _rest; do
    [[ -n ${folder:-} ]] || continue
    total=$(( total + 1 ))

    if [[ -z ${pixels_per_mm:-} ]]; then
        echo "[$total] $name: skipped -- row has fewer than six fields" >&2
        failed=$(( failed + 1 ))
        continue
    fi

    echo "[$total] $name"
    if ! printf '%s\n' "$folder" "$name" "$length" "$frequency" "$fps" "$pixels_per_mm" \
        | "$binary"
    then
        echo "[$total] $name: FAILED" >&2
        failed=$(( failed + 1 ))
    fi
done < "$input_table"

echo "processed $total experiment(s), $failed failed"
(( failed == 0 ))
