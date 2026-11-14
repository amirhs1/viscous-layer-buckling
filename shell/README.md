# Batch Driver and Plotting Scripts

Shell and gnuplot scripts that drive the [OpenCV interface curvature
pipeline](../opencv/README.md) across the whole experiment dataset, and
plot its output. The analysis itself lives in the OpenCV project; this
folder is the layer around it — assembling per-experiment inputs,
running the 70 experiments unattended, and turning the results into
figures.

Originally a `commands.txt` of shell one-liners kept as a lab notebook,
plus two gnuplot fragments. Reconstructed here as runnable scripts.

Part of [this repository](../README.md), alongside the [OpenCV
pipeline](../opencv/README.md) it drives and the [gnuplot
figures](../gnuplot/README.md).

## Layout

```
.
├── bin/     # workflow shell scripts, numbered in run order
├── plots/   # gnuplot scripts
└── work/    # lists and batch table written by step 3 (created on first run, ignored by git)
```

## Workflow

| Step | Script | Purpose |
|---|---|---|
| 1 | [`bin/01_renumber_frames.sh`](bin/01_renumber_frames.sh) | Rename raw camera frames into the `-Renumbered-0001.tif` form stage 2 expects. |
| 2 | [`bin/02_make_stage_dirs.sh`](bin/02_make_stage_dirs.sh) | Pre-create a per-stage output directory in every experiment folder. |
| 3 | [`bin/03_build_input_table.sh`](bin/03_build_input_table.sh) | Combine the experiment tree and the lab data sheet into the stage 2 batch input table. |
| 4 | [`bin/04_run_template_matching.sh`](bin/04_run_template_matching.sh) | Feed that table to the stage 2 executable, one experiment per row. |

Every script prints usage with no arguments, and takes the experiment
root as a parameter rather than assuming a working directory.

The workflow stops at OpenCV stage 2 because the next step was not a
script: the frames were thresholded by hand in ImageJ (see *Data*
below), so there was never a batch driver for stages 3–4.

The experiment tree is expected to be:

```
<experiment_root>/
└── <date>/                  # digits and dashes only, e.g. 1394-11-13
    └── <experiment_name>/   # e.g. D1394_11_13-T09_57
        └── <experiment_name>-1-Renumbered/
```

Anything in `<experiment_root>` that is not a date-named directory is
skipped, so notes and stray files can live alongside the data.

### Typical run

```bash
export MSC_DATA=/path/to/data   # the input data folder; there is no default
bin/02_make_stage_dirs.sh ~/Experiment-Videos -5-MinContour
bin/03_build_input_table.sh ~/Experiment-Videos
bin/04_run_template_matching.sh ../opencv/build/template-matching   # MSC_NO_DISPLAY=1 to run without windows
```

Step 1 is per-experiment and only needed for runs whose frames still
carry the camera's own naming. It previews renames by default; pass
`--apply` to perform them.

## Plots

Both gnuplot scripts run with no arguments against the reference
data in `$MSC_DATA/curvature-reference/`, and accept overrides via
`gnuplot -e`. Run them from this directory:

```bash
gnuplot plots/mean_curvature.gp     # -> mean_curvature.eps
gnuplot plots/frame_variance.gp     # -> frame_variance.png
```

Both write into the current directory. Like the gnuplot figures, their output
is not kept here — pass `outfile` to send it elsewhere.

- [`plots/mean_curvature.gp`](plots/mean_curvature.gp) — smoothed mean
  curvature against normalized frame number. Defaults to overlaying the
  two smoothings (B-spline and moving-average) of one experiment; point
  `series_a`/`series_b` at two stage 3 `-SmoothedData.txt` files to compare
  experiments instead (stage 4's files use other columns).
- [`plots/frame_variance.gp`](plots/frame_variance.gp) — per-frame
  variance of the interface profile against normalized frame number,
  from `-FrameStatistics.txt`. This is the amplitude growth curve of
  the buckling instability.

```bash
gnuplot -e "series_a='A-SmoothedData.txt'; label_a='4.1 Hz'; \
            series_b='B-SmoothedData.txt'; label_b='8.8 Hz'; \
            outfile='comparison.eps'" plots/mean_curvature.gp
```

## Data

The input data are not part of this repository (see *Data availability*
in the [main README](../README.md)); paths below are relative to `MSC_DATA`.

`experiment_report_data_sheet.csv` is the hand-kept lab record and
the only file in the data that is a primary source — everything else
is either derived from it or produced by the pipeline.

It is **transposed**: each row is one measured quantity and each column
one of the 70 experiments. Step 3 pulls four rows out of it — `Li (mm)`,
`Frequency (Hz)`, `Frame per Second`, and `pixels per millimetre` — and
turns each into a column of the batch table.

It is also the only record of the **ImageJ step** that sits between
OpenCV stages 2 and 3. The `IMAGE PROCESSING` block (rows 143–149)
holds the parameters of the background subtraction and thresholding
that produced the `-3-Resized/` and `-4-Thershold/` sequences the
OpenCV pipeline reads but never writes:

| Row | ImageJ dialog | Values (exp. 1–4) |
|---|---|---|
| `Rolling Ball Radius` | Process ▸ Subtract Background | 3, 5, 5, 5 |
| `Separated Colours` | " (separate colors) | Yes ×4 |
| `Disable Smoothing` | " (disable smoothing) | Yes ×4 |
| `Threshold Method` | Image ▸ Adjust ▸ Threshold | Li ×4 |
| `Min/Max` | " (threshold bounds) | ???, 0/22, 0/20, 0/9 |
| `Stack Histogram` | " (stack histogram) | No, Yes, Yes, No |

Only the first four columns were ever filled in, and experiment 1's
bounds are recorded as `???`. The method is therefore known for the
whole dataset, but the exact threshold bounds survive for experiments
2–4 only — see the workflow-gap note in
[`../opencv/README.md`](../opencv/README.md).

`batch-inputs/` holds those extracted lists plus the paths walked from
the experiment tree. All of them are regenerated by step 3 (into
`work/`); they are kept with the data because the original experiment
tree was not kept, so the lists are the record of which experiments were
processed and of where their frames once lived. The `IMAGE PROCESSING`
block above is the only other trace of that tree, and it covers just
four of the 70 experiments.

- `experiment_names.txt`, `experiment_paths.txt`, `image_sequence_paths.txt`
- `initial_length_mm.txt`, `motor_frequency_hz.txt`, `frames_per_second.txt`, `pixels_per_mm.txt`
- `stage2_input_table_original.txt` — the table as actually used in
  2016, preserved apart from the folder prefix of its paths (see *Corrections* below)

`curvature-reference/` is stage 3/4 output for a single experiment, kept as
reference data so the plotting scripts are runnable without rerunning
the pipeline: `frame_statistics.txt`, `spline.txt`,
`moving_average.txt`, `spline_second_derivative.txt`, `fft.txt`,
and `fit_parameters.txt`.

`regression/` holds the aggregate result in two versions — one row per
experiment giving motor frequency against fitted compression velocity
and its error, the input to the frequency–velocity regression.

> The two are the **opencv copy**, `regression/linear_regression_data.txt`,
> which stage 2 appended to at runtime, and the **shell copy**,
> `regression/linear_regression_data-shell-copy.txt`, once kept with these
> scripts. They are not identical: the copies differ in row order, and in
> the velocity and error reported for `D1394_11_24-T19_19`. **The opencv
> copy is the good one** — see below. Neither file has been edited, so the
> superseded value is still present in the shell copy.

### Which `D1394_11_24-T19_19` velocity is correct

The opencv copy's `-135.095 mm/s` (error `-0.0151479`). The shell
copy reports `-127.592 mm/s` (error `-1.14676`) for the same
run, from an earlier pass over a bad fit window.

Compression velocity is very tightly linear in motor frequency across
the dataset. Fitting the other 69 experiments gives

```
velocity = -4.49531 * frequency + 1.40718   mm/s     (residual SD 0.404)
```

which predicts `-134.801 mm/s` at this run's 30.3 Hz. Against that:

| value | residual | |
|---|---|---|
| `-135.095` (opencv copy) | −0.294 mm/s | 0.7 SD |
| `-127.592` (shell copy) | +7.209 mm/s | 17.8 SD |

For scale, the worst residual among the other 69 experiments is
2.6 SD. The reported error tells the same story: `-1.14676` is 17 times
larger than the next largest in the file, so that fit was already
flagged as bad by the program itself.

The cause is visible in the data sheet. Stage 2 fits every frame it is
handed, so the fit window is whatever sits in the `-1-Renumbered`
folder. This run's recorded window is 80 frames, but its geometry
(175 mm to 37 mm at ~135 mm/s, 60.04 fps) accounts for only ~61 frames
of actual wall motion — roughly 19 stationary frames were included.
Padding a synthetic fit with that many stationary frames, split between
the two ends, reproduces both numbers:

| | slope (mm/s) | cov01 |
|---|---|---|
| clean window | −135.095 | — |
| ~19 stationary frames added | −125.7 | −0.999 |
| reported in the shell copy | −127.592 | −1.14676 |

Row order corroborates it. Stage 2 *appends* one row per run, and the
opencv copy is in exact chronological order, whereas the shell copy is in
chronological order except for `D1394_11_21-T12_11`, appended at the
end as a re-run. The shell copy is therefore the working log; the opencv
copy is the tidied final version, and it holds the corrected value.

## Corrections

Four defects in the original scripts are fixed here. They are listed
because the historical files are kept with the data (`batch-inputs/`) alongside the corrected
ones, and the difference matters if the old outputs are ever reused.

**Column order in the batch table.** Stage 2 takes no arguments; it
reads six whitespace-separated values from stdin in this order:

```
experiment_folder  experiment_name  initial_length  motor_frequency  fps  pixels_per_mm
```

The original `paste exp exp-path l.txt fre.txt f.txt p.txt` put the
name before the path, and the surviving table (now `stage2_input_table_original.txt`) carries a seventh
leading column — the `-1-Renumbered/...%04d.tif` sequence pattern —
left over from an earlier design in which the path was an `argv`
argument. Stage 2 now builds that path itself from folder and name and
never reads it. Fed that table, stage 2 consumes the sequence pattern
as the folder, the name as the name, and then hits a path string where
it expects a number for the initial length, so the numeric read fails
and every subsequent value is skewed. Step 3 emits six columns in the
order above; the seven-column original is kept as
`batch-inputs/stage2_input_table_original.txt` in the data and is **not** usable as
input.

**Curly quotes in the curvature plot.** The original `set output` line
used typographic quotes (`‘curvature.eps’`), which gnuplot rejects.

**Unterminated quote and placeholder body in the variance plot.** The
original had `set output 'variance-frame.png` with no closing quote,
and its only plot command was `plot sin(x)` — a stub that was never
finished. `plots/frame_variance.gp` plots the real variance column.

**Missing referenced files.** `commands.txt` referred to `exp` and
`argu.txt`, neither of which survives. `exp` was the experiment-name
list, now `experiment_names.txt`, regenerated by step 3 into `work/`.
The `argu.txt` loop was an abandoned attempt to pass arguments to a
stage 2 that no longer accepted them, superseded in the same file by
the row-at-a-time stdin loop that step 4 implements.

## Requirements

See *Requirements* in the [main README](../README.md); step 4 also needs the stage 2
executable from the [OpenCV project](../opencv/README.md).

`dos2unix` is no longer needed — step 3 strips the data sheet's DOS
line endings itself.
