#!/usr/bin/env bash
#
# Assemble the batch input table that drives stage 2 of the OpenCV
# pipeline (template-matching) over every experiment in the dataset.
#
# Two sources are combined positionally:
#
#   1. The experiment tree on disk, laid out as
#      <root>/<date>/<experiment_name>/, which gives the folder path,
#      the experiment name, and the renumbered image-sequence pattern.
#
#   2. The lab data sheet, a *transposed* CSV: each row is one measured
#      quantity and each column one experiment. Four rows are pulled
#      out here -- initial length, motor frequency, FPS, and the
#      pixels-per-mm scale.
#
# The two sources are joined BY POSITION, not by name: the data sheet's
# "Movie Name" row uses a different convention (D13_11_1394-T9_57) from
# the folder names (D1394_11_13-T09_57), so they cannot be matched on a
# key. The script therefore checks that both sources yield the same
# number of experiments and refuses to emit a table if they disagree.

set -euo pipefail

usage () {
    cat <<'USAGE'
Usage: 03_build_input_table.sh <experiment_root> [data_sheet.csv] [output_dir]

  <experiment_root>  directory containing the date-named experiment folders
  [data_sheet.csv]   lab data sheet (default: $MSC_DATA/experiment_report_data_sheet.csv)
  [output_dir]       where to write the lists (default: ../work, created if missing)

Writes, into <output_dir>:
  experiment_names.txt       experiment folder names
  experiment_paths.txt       absolute path to each experiment folder
  image_sequence_paths.txt   the -1-Renumbered/...-%04d.tif pattern per experiment
  initial_length_mm.txt      data sheet row "Li (mm)"
  motor_frequency_hz.txt     data sheet row "Frequency (Hz)"
  frames_per_second.txt      data sheet row "Frame per Second"
  pixels_per_mm.txt          data sheet row "pixels per millimetre"
  stage2_input_table.txt     the six columns above in stage 2's stdin order

Example:
  03_build_input_table.sh ~/Experiment-Videos
USAGE
}

if [[ $# -lt 1 || $# -gt 3 ]]; then
    usage >&2
    exit 1
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
experiment_root=$1
# The data sheet comes from argument 2 or from $MSC_DATA; there is no default,
# because the data are not part of this repository.
if [[ $# -ge 2 ]]; then
    data_sheet=$2
elif [[ -n ${MSC_DATA:-} ]]; then
    data_sheet=$MSC_DATA/experiment_report_data_sheet.csv
else
    cat >&2 <<'EOF'
error: MSC_DATA is not set.
Set it to the folder that holds the input data, for example:
    export MSC_DATA=/path/to/data
The data are not part of this repository; see "Data availability" in README.md.
EOF
    exit 1
fi
output_dir=${3:-$script_dir/../work}

[[ -d $experiment_root ]] || { echo "error: no such directory: $experiment_root" >&2; exit 1; }
[[ -f $data_sheet ]] || { echo "error: no such file: $data_sheet" >&2; exit 1; }
[[ $# -ge 3 ]] || mkdir -p "$output_dir"
[[ -d $output_dir ]] || { echo "error: no such directory: $output_dir" >&2; exit 1; }

experiment_root=$(cd -- "$experiment_root" && pwd)

# --- 1. Walk the experiment tree -------------------------------------
# Date folders are digits and dashes only (e.g. 1394-11-13); each holds
# one folder per experiment.
: > "$output_dir/experiment_names.txt"
: > "$output_dir/experiment_paths.txt"
: > "$output_dir/image_sequence_paths.txt"

for date_dir in "$experiment_root"/*; do
    [[ -d $date_dir ]] || continue
    [[ ${date_dir##*/} =~ ^[-0-9]+$ ]] || continue

    for exp_dir in "$date_dir"/*; do
        [[ -d $exp_dir ]] || continue
        exp_name=${exp_dir##*/}
        printf '%s\n' "$exp_name" >> "$output_dir/experiment_names.txt"
        printf '%s\n' "$exp_dir"  >> "$output_dir/experiment_paths.txt"
        printf '%s/%s-1-Renumbered/%s-Renumbered-%%04d.tif\n' \
            "$exp_dir" "$exp_name" "$exp_name" \
            >> "$output_dir/image_sequence_paths.txt"
    done
done

# --- 2. Pull the measured columns out of the data sheet --------------
# The sheet is transposed, so one row becomes one column of the table.
# Matching on "<label>," anchors to a complete field and avoids hitting
# longer labels that merely start the same way (e.g. "Frame per Second"
# vs "Frame per Second - Error"). Trailing CRs are stripped -- the sheet
# was exported from Excel with DOS line endings.
extract_row () {
    local label=$1 destination=$2
    grep -m1 -F "$label," "$data_sheet" \
        | tr ',' '\n' \
        | sed '1d' \
        | tr -d '\r' \
        > "$destination"

    if [[ ! -s $destination ]]; then
        echo "error: data sheet row not found: $label" >&2
        exit 1
    fi

    # Stage 2 reads these as numbers; a blank cell would leave it
    # reading the next row's first field instead, silently shifting
    # every remaining value. Refuse rather than emit a skewed table.
    local blanks
    blanks=$(grep -c '^[[:space:]]*$' "$destination" || true)
    if (( blanks > 0 )); then
        echo "error: data sheet row '$label' has $blanks empty cell(s)." >&2
        echo "       Fill them in $data_sheet before rebuilding." >&2
        exit 1
    fi
}

extract_row 'Li (mm)'               "$output_dir/initial_length_mm.txt"
extract_row 'Frequency (Hz)'        "$output_dir/motor_frequency_hz.txt"
extract_row 'Frame per Second'      "$output_dir/frames_per_second.txt"
extract_row 'pixels per millimetre' "$output_dir/pixels_per_mm.txt"

# --- 3. Check the two sources line up --------------------------------
columns=(
    experiment_paths.txt
    experiment_names.txt
    initial_length_mm.txt
    motor_frequency_hz.txt
    frames_per_second.txt
    pixels_per_mm.txt
)

expected=$(( $(wc -l < "$output_dir/experiment_paths.txt") ))   # $(( )) drops the padding BSD wc adds
for column in "${columns[@]}"; do
    actual=$(( $(wc -l < "$output_dir/$column") ))
    if (( actual != expected )); then
        echo "error: $column has $actual rows, expected $expected." >&2
        echo "       The tree and the data sheet are joined by position," >&2
        echo "       so every column must have one row per experiment." >&2
        exit 1
    fi
done

# --- 4. Emit the table in stage 2's stdin order ----------------------
# Stage 2 reads six whitespace-separated values in this exact order:
#   experiment folder, experiment name, initial length, motor
#   frequency, FPS, pixels per mm.
# It derives the image-sequence path itself from folder + name, so
# image_sequence_paths.txt is NOT part of the table.
( cd "$output_dir" && paste "${columns[@]}" > stage2_input_table.txt )

echo "wrote $output_dir/stage2_input_table.txt ($expected experiments)"
