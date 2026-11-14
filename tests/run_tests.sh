#!/usr/bin/env bash
# Run the test suite: build the OpenCV stages and the input generator, generate synthetic inputs
# (no real data needed), run every stage, the shell steps and the gnuplot table extraction, and
# check the results against values known from the inputs.
#
# Usage: tests/run_tests.sh
# Needs only what the repository already needs: CMake, a C++ compiler, OpenCV, GSL, Boost,
# gnuplot and Bash (awk does the arithmetic). Everything is written under tests/work/ (ignored by
# git), which each run starts afresh.
set -uo pipefail
HERE=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO=$(dirname "$HERE")
W=$HERE/work
export MSC_NO_DISPLAY=1          # no OpenCV windows
unset MSC_DATA                   # the tests never read the real data
rm -rf "${W:?}"; mkdir -p "$W"

pass=0; fail=0
check () {   # description, condition (evaluated)
    if eval "$2"; then echo "  PASS  $1"; pass=$((pass + 1)); else echo "  FAIL  $1"; fail=$((fail + 1)); return 1; fi
}
near () {    # value, expected, tolerance
    awk -v a="$1" -v b="$2" -v t="$3" 'BEGIN { d = a - b; if (d < 0) d = -d; exit !(a != "" && d <= t) }'
}
same_stats () {   # stage (3 or 4), expected.txt, FrameStatistics.txt, mm per pixel
    # stage 3: frame from 1, amplitude in column 5, variance in column 6
    # stage 4: frame from 0, amplitude in column 3, variance in column 7
    awk -F'\t' -v stage="$1" -v scale="$4" '
        function abs(x) { return x < 0 ? -x : x }
        function off(got, want) { return abs(got - want) > 1e-9 + 1e-5 * abs(want) }
        FNR == NR { if (FNR > 1) { amp[$1] = $2 * scale; var[$1] = $3 * scale * scale; n++ } next }
        /^#/ || NF == 0 { next }
        {
            f = stage == 4 ? $1 + 1 : $1; a = stage == 4 ? $3 : $5; v = stage == 4 ? $7 : $6
            if (!(f in amp) || off(a, amp[f]) || off(v, var[f])) {
                printf "    frame %d: amplitude %s, variance %s; expected %g, %g\n", f, a, v, amp[f], var[f]; bad = 1; exit
            }
            seen++
        }
        END { if (bad) exit 1; if (seen != n) { printf "    %d frames, expected %d\n", seen, n; exit 1 } }' "$2" "$3"
}
show_log () {   # file: print its last lines, to see why a stage failed
    echo "    --- last lines of ${1#"$W"/}:"; tail -n 8 "$1" | sed 's/^/    /'
}
stage34_inputs () {   # experiment folder, mode
    local name=${1##*/}
    mkdir -p "$1/$name-3-Resized" "$1/$name-4-Thershold" "$1/$name-5-MinContour"
    "$GEN" stage34 "$1" "$2"
}

echo "build (opencv/ and the input generator)"
out=$(cmake -S "$REPO/opencv" -B "$W/build-no-data" 2>&1); rc=$?
check "configuring without MSC_DATA_DIR stops with a message" '[[ $rc != 0 && $out == *"MSC_DATA_DIR is not set"* ]]'
cmake -S "$HERE" -B "$W/build-inputs" > "$W/inputs-build.log" 2>&1 &&
    cmake --build "$W/build-inputs" >> "$W/inputs-build.log" 2>&1; rc=$?
check "the input generator builds" '[[ $rc == 0 ]]'
GEN=$W/build-inputs/make-inputs
DATA=$W/data
mkdir -p "$DATA/marker-templates" && "$GEN" markers "$DATA/marker-templates"
cmake -S "$REPO/opencv" -B "$W/build" -DMSC_DATA_DIR="$DATA" > "$W/configure.log" 2>&1 &&
    cmake --build "$W/build" -j 4 > "$W/build.log" 2>&1; rc=$?
check "the four stages configure and build" '[[ $rc == 0 ]]'
if (( rc != 0 )) || [[ ! -x $GEN ]]; then tail -20 "$W/build.log" "$W/inputs-build.log"; echo "result: build failed"; exit 1; fi
B=$W/build

echo "stage 1: background-template-prototype"
S12=$W/stage12
E=$S12/0000-00-00/D0000_00_00-T00_00
mkdir -p "$E/D0000_00_00-T00_00-1-Renumbered" && "$GEN" stage12 "$E" "$DATA/marker-templates"
SEQ=$E/D0000_00_00-T00_00-1-Renumbered/D0000_00_00-T00_00-Renumbered-%04d.tif
out=$("$B/background-template-prototype" "$SEQ" 2>&1); rc=$?
check "reads the sequence and prints the frame size" '[[ $rc == 0 && $out == *"[640 x 240]"* ]]'

echo "stage 2: template-matching (markers 2 px per frame each, 60 fps, 6.3 px per mm)"
printf '%s\n' "$E" D0000_00_00-T00_00 175 10 60 6.3 | "$B/template-matching" > "$W/stage2.log" 2>&1; rc=$?
row=$(grep -v '^#' "$B/linear_regression_data.txt" 2> /dev/null | tail -1)
check "exit 0 and one regression row" '[[ $rc == 0 && $(grep -vc "^#" "$B/linear_regression_data.txt") == 1 ]]'
check "velocity -38.0952 mm/s (length shrinks 4 px per frame: -4 * 60 / 6.3)" 'near "$(cut -f7 <<< "$row")" -38.0952 1e-3'
check "scale 0.15873 mm per pixel (1 / 6.3)" 'near "$(cut -f5 <<< "$row")" 0.15873 1e-5'

echo "stage 3: interface-statistics (a sinusoidal interface whose amplitude grows)"
N=D1111_11_11-T11_11
stage34_inputs "$W/stage3/$N" growing
echo 0.1 | "$B/interface-statistics" "$W/stage3/$N" "$N" > "$W/stage3.log" 2>&1; rc=$?
check "exit 0" '[[ $rc == 0 ]]' || show_log "$W/stage3.log"
check "amplitude and variance of every frame match the interface (0.1 mm per pixel)" \
    'same_stats 3 "$W/stage3/$N/expected.txt" "$W/stage3/$N/$N-FrameStatistics.txt" 0.1'
K=D2222_22_22-T22_22
stage34_inputs "$W/stage3-blank/$K" blank
out=$(echo 0.1 | "$B/interface-statistics" "$W/stage3-blank/$K" "$K" 2>&1); rc=$?
check "stops with a message at a frame with no contour" '[[ $rc != 0 && $out == *"no contour in thresholded frame 2"* ]]' ||
    { echo "$out" > "$W/stage3-blank.log"; show_log "$W/stage3-blank.log"; }

echo "stage 4: final-pipeline"
stage34_inputs "$W/stage4/$N" growing
"$B/final-pipeline" "$W/stage4/$N" "$N" 0.1 0 > "$W/stage4.log" 2>&1; rc=$?
check "growing interface, period switch 0: exit 0" '[[ $rc == 0 ]]' || show_log "$W/stage4.log"
check "amplitude and variance of every frame match the interface" \
    'same_stats 4 "$W/stage4/$N/expected.txt" "$W/stage4/$N/$N-FrameStatistics.txt" 0.1'
check "writes the smoothed data and the transition point" '[[ -s $W/stage4/$N/$N-SmoothedData.txt && -s $W/stage4/$N/$N-TransitionPoint.txt ]]'
O=D4444_44_44-T44_44
stage34_inputs "$W/stage4-osc/$O" oscillating
out=$("$B/final-pipeline" "$W/stage4-osc/$O" "$O" 0.1 1 2>&1); rc=$?
check "oscillating interface, period switch 1: reports 25.7143 frames for a 24-frame period (see Known issues)" \
    '[[ $rc == 0 && $out == *"Frequency in frames: 25.7143"* ]]' || { echo "$out" > "$W/stage4-osc.log"; show_log "$W/stage4-osc.log"; }
out=$("$B/final-pipeline" "$W/stage4/$N" "$N" 0.1 2>&1); rc=$?
check "without the period switch: prints its usage and exits non-zero" '[[ $rc != 0 && $out == *"Usage: final-pipeline"* ]]'

echo "shell/bin: steps 1-4 on a made-up experiment tree"
T=$W/tree
mkdir -p "$T/1394-11-13/D1394_11_13-T09_57" "$T/1394-11-13/D1394_11_13-T16_55" "$T/1394-11-14/D1394_11_14-T08_55" \
         "$T/misc-not-a-date/D9999_99_99-T00_00"
touch "$T/notes.txt"
F=$T/1394-11-13/D1394_11_13-T16_55/D1394_11_13-T16_55-1-Renumbered
mkdir -p "$F"; for i in 001 002 003; do touch "$F/D1394_11_13-T16_55-$i.tif"; done
out=$("$REPO/shell/bin/01_renumber_frames.sh" "$F" "_55-" "_55-Renumbered-0")
check "step 1 dry run lists 3 renames and changes nothing" '[[ $out == *"3 file(s) would be renamed"* && -e $F/D1394_11_13-T16_55-001.tif ]]'
"$REPO/shell/bin/01_renumber_frames.sh" --apply "$F" "_55-" "_55-Renumbered-0" > /dev/null
check "step 1 --apply renames to -Renumbered-0001.tif .. 0003" '[[ -e $F/D1394_11_13-T16_55-Renumbered-0001.tif && -e $F/D1394_11_13-T16_55-Renumbered-0003.tif && ! -e $F/D1394_11_13-T16_55-001.tif ]]'
touch "$F/D1394_11_13-T16_55-004.tif" "$F/D1394_11_13-T16_55-Renumbered-0004.tif"
"$REPO/shell/bin/01_renumber_frames.sh" --apply "$F" "_55-" "_55-Renumbered-0" > /dev/null 2>&1; rc=$?
check "step 1 refuses to overwrite an existing file" '[[ $rc == 1 && -e $F/D1394_11_13-T16_55-004.tif ]]'
out=$("$REPO/shell/bin/02_make_stage_dirs.sh" "$T" -5-MinContour)
check "step 2 makes one -5-MinContour folder per experiment and skips the non-date folder" \
    '[[ $out == *"ensured 3 "* && -d $T/1394-11-14/D1394_11_14-T08_55/D1394_11_14-T08_55-5-MinContour && ! -d $T/misc-not-a-date/D9999_99_99-T00_00/D9999_99_99-T00_00-5-MinContour ]]'

sheet () {   # output, then the values of Li, Frequency, FPS and pixels per mm, one comma list each
    printf '%s\r\n' "Number,1,2,3" "Movie Name,D13_11_1394-T9_57,D13_11_1394-T16_55,D14_11_1394-T8_55" \
        "Li (mm),$2" "Frequency (Hz),$3" "Frame per Second - Error,1,1,1" "Frame per Second,$4" \
        "pixels per millimetre,$5" > "$1"
}
sheet "$W/sheet.csv" 175,200,150 10,20,30 60,60,30 6.3,6.3,6.4
L=$W/lists
out=$(env -u MSC_DATA "$REPO/shell/bin/03_build_input_table.sh" "$T" 2>&1); rc=$?
check "step 3 without MSC_DATA or a data sheet stops with a message" '[[ $rc == 1 && $out == *"MSC_DATA is not set"* ]]'
mkdir -p "$L"
out=$("$REPO/shell/bin/03_build_input_table.sh" "$T" "$W/sheet.csv" "$L" 2>&1); rc=$?
check "step 3 builds a table of 3 experiments" '[[ $rc == 0 && $out == *"(3 experiments)"* ]]'
check "step 3 rows: folder, name, length, frequency, fps, px/mm (data sheet CRs removed)" \
    '[[ $(cat "$L/stage2_input_table.txt") == "$T/1394-11-13/D1394_11_13-T09_57	D1394_11_13-T09_57	175	10	60	6.3
$T/1394-11-13/D1394_11_13-T16_55	D1394_11_13-T16_55	200	20	60	6.3
$T/1394-11-14/D1394_11_14-T08_55	D1394_11_14-T08_55	150	30	30	6.4" ]]'
check "step 3 writes the image-sequence pattern of each experiment" \
    '[[ $(head -1 "$L/image_sequence_paths.txt") == "$T/1394-11-13/D1394_11_13-T09_57/D1394_11_13-T09_57-1-Renumbered/D1394_11_13-T09_57-Renumbered-%04d.tif" ]]'
sheet "$W/sheet-blank.csv" 175,,150 10,20,30 60,60,30 6.3,6.3,6.4
out=$("$REPO/shell/bin/03_build_input_table.sh" "$T" "$W/sheet-blank.csv" "$L" 2>&1); rc=$?
check "step 3 refuses a data sheet with an empty cell" '[[ $rc == 1 && $out == *"empty cell"* ]]'
sheet "$W/sheet-short.csv" 175,200 10,20 60,60 6.3,6.3
out=$("$REPO/shell/bin/03_build_input_table.sh" "$T" "$W/sheet-short.csv" "$L" 2>&1); rc=$?
check "step 3 refuses a data sheet with fewer experiments than the tree" '[[ $rc == 1 && $out == *"has 2 rows, expected 3"* ]]'
"$REPO/shell/bin/03_build_input_table.sh" "$T" "$W/sheet.csv" "$L" > /dev/null

cat > "$W/fake-stage2" <<'EOF'
#!/usr/bin/env bash
# stands in for stage 2: logs the six values it is given and fails for one experiment
read -r folder; read -r name; read -r length; read -r freq; read -r fps; read -r ppm
printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$folder" "$name" "$length" "$freq" "$fps" "$ppm" >> "$(dirname "$0")/fake-stage2.log"
[[ $name != D1394_11_13-T16_55 ]]
EOF
chmod +x "$W/fake-stage2"
out=$("$REPO/shell/bin/04_run_template_matching.sh" "$W/fake-stage2" "$L/stage2_input_table.txt" 2>&1); rc=$?
check "step 4 runs every experiment, counts the failure and exits non-zero" '[[ $out == *"processed 3 experiment(s), 1 failed"* && $rc != 0 ]]'
check "step 4 passes each row's six values in order" 'cmp -s "$W/fake-stage2.log" "$L/stage2_input_table.txt"'

echo "shell/bin steps 3-4 with the real stage 2 (the stage 1-2 experiment)"
L2=$W/lists-stage2; mkdir -p "$L2"
printf '%s\r\n' "Number,1" "Li (mm),175" "Frequency (Hz),10" "Frame per Second,60" "pixels per millimetre,6.3" > "$W/sheet-one.csv"
"$REPO/shell/bin/03_build_input_table.sh" "$S12" "$W/sheet-one.csv" "$L2" > /dev/null 2>&1
out=$("$REPO/shell/bin/04_run_template_matching.sh" "$B/template-matching" "$L2/stage2_input_table.txt" 2>&1); rc=$?
row=$(grep -v '^#' "$B/linear_regression_data.txt" 2> /dev/null | tail -1)
check "the batch run appends a second regression row with the same velocity" \
    '[[ $rc == 0 && $(grep -vc "^#" "$B/linear_regression_data.txt") == 2 ]] && near "$(cut -f7 <<< "$row")" -38.0952 1e-3'

echo "gnuplot/"
cat > "$W/regression.txt" <<'EOF'
#1:Experiment Name		2:Experiment Path		3:Initial Length		4:Frame per seconds		5:Milimeters per pixel		6:Frequency (Hz)		7:Velocity(mm/s)		8:Velocity Error(mm/s)
D0000_00_00-T00_00	/somewhere/D0000_00_00-T00_00	175	60	0.15873	10	-38.0952	-0.25
D0000_00_00-T00_01	/somewhere/D0000_00_00-T00_01	175	60	0.15873	20	-12.5	-0.0004
EOF
( cd "$REPO/gnuplot/velocity_frequency" && gnuplot -e "INPUT='$W/regression.txt'; OUTPUT='$W/velocity-frequency.txt'" extract_data.gp ) > "$W/extract.log" 2>&1; rc=$?
check "extract_data.gp turns the regression table into frequency, 0.2, -velocity, sqrt(-error)" \
    '[[ $rc == 0 && $(cat "$W/velocity-frequency.txt") == "Frequency	Frequency_error	Velocity	Velocity_error
10	0.2	38.0952	0.5
20	0.2	12.5	0.02" ]]'
out=$(cd "$REPO/gnuplot/velocity_frequency" && gnuplot extract_data.gp 2>&1); rc=$?
check "extract_data.gp without MSC_DATA or INPUT stops with a message" '[[ $rc != 0 && $out == *"MSC_DATA is not set"* ]]'
out=$(cd "$REPO/gnuplot" && ./render.sh 2>&1); rc=$?
check "render.sh without MSC_DATA stops with a message" '[[ $rc != 0 && $out == *"MSC_DATA"* ]]'

echo "result: $pass passed, $fail failed"
(( fail == 0 ))
