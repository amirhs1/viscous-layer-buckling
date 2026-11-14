#!/usr/bin/env bash
#
# render.sh - batch-render the gnuplot examples in this folder.
#
#   ./render.sh              render every script
#   ./render.sh 04 09        render only scripts whose name starts with 04 / 09
#   ./render.sh --png        also convert each EPS/PS to PNG (needs ghostscript)
#   ./render.sh --clean      remove generated images and exit
#
# The input data are not part of this repository. Set MSC_DATA to the folder
# that holds them (see README.md, "Data availability"); every script reads
# from there and writes to images/.

set -uo pipefail
START_DIR=$PWD
cd "$(dirname "$0")"

OUT_DIR="images"
WANT_PNG=0
declare -a FILTERS=()

for arg in "$@"; do
  case "$arg" in
    --png)   WANT_PNG=1 ;;
    --clean) rm -rf "$OUT_DIR" fit.log; echo "removed $OUT_DIR/ and fit.log"; exit 0 ;;
    -h|--help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*) echo "unknown option: $arg" >&2; exit 2 ;;
    *)  FILTERS+=("$arg") ;;
  esac
done

command -v gnuplot >/dev/null || { echo "error: gnuplot not found (brew install gnuplot)" >&2; exit 1; }
# Locate the input data. There is no default: the data are not part of this repository.
if [ -z "${MSC_DATA:-}" ]; then
  cat >&2 <<'EOF'
error: MSC_DATA is not set.
Set it to the folder that holds the input data, for example:
    export MSC_DATA=/path/to/data
The data are not part of this repository; see "Data availability" in README.md.
EOF
  exit 2
fi
DATA_DIR=$(cd "$START_DIR" && cd "$MSC_DATA" 2>/dev/null && pwd) && [ -d "$DATA_DIR/series" ] \
  || { echo "error: MSC_DATA=$MSC_DATA is not the input data folder (it has no series/ folder)" >&2; exit 2; }

mkdir -p "$OUT_DIR"

# Collect scripts, honouring any numeric filters given on the command line.
declare -a SCRIPTS=()
for f in [0-9][0-9]-*.txt; do
  [ -e "$f" ] || continue
  if [ ${#FILTERS[@]} -eq 0 ]; then
    SCRIPTS+=("$f")
  else
    for want in "${FILTERS[@]}"; do
      case "$f" in "$want"*) SCRIPTS+=("$f"); break ;; esac
    done
  fi
done

[ ${#SCRIPTS[@]} -gt 0 ] || { echo "no matching scripts" >&2; exit 1; }

ok=0; failed=0
declare -a FAILED_NAMES=()

# Figures driven by the parameterized overlay.gp, one row per figure.
if [ -f figures.tsv ]; then
  while IFS=$'\t' read -r name data col meta ylab ttl key; do
    case "${name:-}" in ''|'#'*) continue ;; esac
    if [ ${#FILTERS[@]} -gt 0 ]; then
      hit=0
      for want in "${FILTERS[@]}"; do
        case "$name" in "$want"*) hit=1; break ;; esac
      done
      [ "$hit" -eq 1 ] || continue
    fi
    printf '  %-42s ' "$name (overlay)"
    if err=$(gnuplot -e "DATA='$DATA_DIR/$data'; COL=$col; META='$meta'; OUT='$OUT_DIR/$name.eps'; \
                         YLAB='$ylab'; TTL='$ttl'; KEY='$key'" overlay.gp 2>&1) \
       && [ -s "$OUT_DIR/$name.eps" ]; then
      echo "ok"; ok=$((ok + 1))
    else
      echo "FAILED"; printf '%s\n' "$err" | sed 's/^/      /'
      failed=$((failed + 1)); FAILED_NAMES+=("$name")
    fi
  done < figures.tsv
fi

for f in "${SCRIPTS[@]}"; do
  printf '  %-42s ' "$f"
  if err=$(gnuplot -e "DATA_DIR='$DATA_DIR'" "$f" 2>&1); then
    echo "ok"
    ok=$((ok + 1))
  else
    echo "FAILED"
    # Indent gnuplot's complaint so it is obvious which script it belongs to.
    printf '%s\n' "$err" | sed 's/^/      /'
    failed=$((failed + 1))
    FAILED_NAMES+=("$f")
  fi
done

if [ "$WANT_PNG" -eq 1 ]; then
  if command -v gs >/dev/null; then
    echo
    echo "converting to PNG..."
    for img in "$OUT_DIR"/*.eps "$OUT_DIR"/*.ps; do
      [ -e "$img" ] || continue
      gs -dSAFER -dBATCH -dNOPAUSE -dEPSCrop -sDEVICE=png16m -r150 \
         -sOutputFile="${img%.*}.png" "$img" >/dev/null 2>&1 \
         && echo "  $(basename "${img%.*}.png")"
    done
  else
    echo "warning: --png needs ghostscript (brew install ghostscript); skipping" >&2
  fi
fi

echo
echo "rendered $ok/$((ok + failed)) into $OUT_DIR/"
if [ "$failed" -gt 0 ]; then
  echo "failed: ${FAILED_NAMES[*]}"
  exit 1
fi
