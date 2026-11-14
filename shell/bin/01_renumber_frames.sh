#!/usr/bin/env bash
#
# Bulk-rename the raw frames of one experiment into the
# "<experiment_name>-Renumbered-0001.tif" form that stage 2 of the
# OpenCV pipeline expects.
#
# The camera wrote frames under its own naming scheme; this rewrites a
# fixed substring of every filename in a directory. Originally a
# throwaway one-liner hard-coded to a single experiment
# ("_55-" -> "_55-Renumbered-0"); it is parameterised here so the same
# rename can be replayed for any run.
#
# Renames are previewed by default. Pass --apply to actually move files.

set -euo pipefail

usage () {
    cat <<'USAGE'
Usage: 01_renumber_frames.sh [--apply] <frame_dir> <old_substring> <new_substring>

  <frame_dir>       directory holding the frames to rename
  <old_substring>   literal substring to replace in each filename
  <new_substring>   its replacement

Options:
  --apply   perform the renames (default is a dry run that only prints them)

Example:
  01_renumber_frames.sh --apply \
      ~/Experiment-Videos/1394-11-13/D1394_11_13-T16_55 "_55-" "_55-Renumbered-0"
USAGE
}

apply=0
if [[ ${1-} == --apply ]]; then
    apply=1
    shift
fi

if [[ $# -ne 3 ]]; then
    usage >&2
    exit 1
fi

frame_dir=$1
old=$2
new=$3

if [[ ! -d $frame_dir ]]; then
    echo "error: no such directory: $frame_dir" >&2
    exit 1
fi

renamed=0
for path in "$frame_dir"/*; do
    [[ -f $path ]] || continue
    name=${path##*/}
    # Replace only the first occurrence, matching the original one-liner.
    target=${name/"$old"/"$new"}
    [[ $target != "$name" ]] || continue

    if [[ -e $frame_dir/$target ]]; then
        echo "error: refusing to overwrite existing file: $target" >&2
        exit 1
    fi

    if (( apply )); then
        mv -- "$path" "$frame_dir/$target"
    else
        echo "$name -> $target"
    fi
    renamed=$(( renamed + 1 ))
done

if (( apply )); then
    echo "renamed $renamed file(s) in $frame_dir"
else
    echo "dry run: $renamed file(s) would be renamed; re-run with --apply"
fi
