#!/usr/bin/env bash
#
# Create a per-stage output directory inside every experiment folder.
#
# Stages of the OpenCV pipeline write into sibling directories named
# "<experiment_name>-<n>-<Stage>" (-2-TemMat, -3-Resized, -4-Thershold,
# -5-MinContour). Stages 3 and 4 do NOT create their own output
# directory, so it must exist beforehand or contour frames are silently
# not written -- this script pre-creates it across the whole dataset.
#
# The experiment root is laid out as:
#   <root>/<date>/<experiment_name>/
# where <date> directories are named with digits and dashes only
# (e.g. 1394-11-13), which is how they are selected here.

set -euo pipefail

usage () {
    cat <<'USAGE'
Usage: 02_make_stage_dirs.sh <experiment_root> <stage_suffix>

  <experiment_root>  directory containing the date-named experiment folders
  <stage_suffix>     suffix to append to each experiment name,
                     e.g. -3-Resized or -5-MinContour

Example:
  02_make_stage_dirs.sh ~/Experiment-Videos -5-MinContour
USAGE
}

if [[ $# -ne 2 ]]; then
    usage >&2
    exit 1
fi

experiment_root=$1
stage_suffix=$2

if [[ ! -d $experiment_root ]]; then
    echo "error: no such directory: $experiment_root" >&2
    exit 1
fi

created=0
for date_dir in "$experiment_root"/*; do
    [[ -d $date_dir ]] || continue
    date_name=${date_dir##*/}
    # Date folders are digits and dashes only; skip anything else.
    [[ $date_name =~ ^[-0-9]+$ ]] || continue

    for exp_dir in "$date_dir"/*; do
        [[ -d $exp_dir ]] || continue
        exp_name=${exp_dir##*/}
        mkdir -p -- "$exp_dir/$exp_name$stage_suffix"
        created=$(( created + 1 ))
    done
done

echo "ensured $created directory/directories named '*$stage_suffix'"
