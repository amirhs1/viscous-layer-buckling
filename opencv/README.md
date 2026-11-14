# Interface Curvature Tracking Pipeline (OpenCV / GSL / Boost)

C++ pipeline for extracting interface shape statistics — amplitude and
curvature over time — from high-speed video of a fluid or fibre
interface. Developed for MSc research as four sequential stages, built
here as four executables from a single CMake project.

Part of [this repository](../README.md). The [batch drivers](../shell/README.md)
run this pipeline across the whole dataset; [`gnuplot/`](../gnuplot/README.md)
turns its output into the thesis figures.

## Layout

```
.
├── CMakeLists.txt   # builds all four stage executables
└── src/             # one source file per pipeline stage
```

## Pipeline stages

| Stage | Source | Executable | Purpose |
|---|---|---|---|
| 1 | [`src/01_background_template_prototype.cpp`](src/01_background_template_prototype.cpp) | `background-template-prototype` | Prototype: template-match two reference markers per frame and background-subtract consecutive frames to visualize motion. |
| 2 | [`src/02_template_matching.cpp`](src/02_template_matching.cpp) | `template-matching` | Locate the region of interest each frame via template matching and crop the sequence to it. |
| 3 | [`src/03_interface_statistics.cpp`](src/03_interface_statistics.cpp) | `interface-statistics` | Extract per-frame statistics (extrema, variance, FFT, mean curvature) with moving-average and B-spline smoothing. |
| 4 | [`src/04_final_pipeline.cpp`](src/04_final_pipeline.cpp) | `final-pipeline` | Consolidated final version of stage 3, with fit uncertainties — used for reported results. |

Stages 2–4 read image sequences from an experiment folder supplied at
runtime and write tab-separated `.txt` results back into it.

## Dependencies

- CMake >= 2.8 and a C++11 compiler
- [OpenCV](https://opencv.org/) (`videoio`, `highgui`, `imgproc`, `video`)
- [GSL](https://www.gnu.org/software/gsl/) — FFT, B-splines, statistics (stages 2–4)
- [Boost](https://www.boost.org/) `filesystem` (stages 2–4)

## Build

One configure/build produces all four executables:

```bash
mkdir build && cd build
cmake -DMSC_DATA_DIR=/path/to/data ..   # or set MSC_DATA first
make
```

`MSC_DATA_DIR` is the input data folder (see *Data availability* in the
[main README](../README.md)). It has no default: configuring stops if it is not
set. Only stage 1 uses it, for its marker templates.

## Run

**Stage 1** — takes a video or image sequence as its argument. Its
template images are loaded from `marker-templates/` in the input data via a path baked in at compile
time, so it runs correctly from any working directory:

```bash
./background-template-prototype <path-to-video-or-image-sequence>
```

**Stage 2** — prompts for the experiment folder and name, plus initial
length, motor frequency, FPS, and pixels-per-mm ratio:

```bash
./template-matching
```

Expected input layout, with the two markers and the renumbered frames
inside the experiment folder:

```
<experiment_folder>/
├── green.tif                                   # left marker template
├── red.tif                                     # right marker template
└── <experiment_name>-1-Renumbered/
    └── <experiment_name>-Renumbered-0001.tif ...
```

Cropped frames are written to
`<experiment_folder>/<experiment_name>-2-TemMat/`, a per-experiment
length log to `<experiment_name>-Velovity-Log.txt`, and one summary row
per run is appended to `linear_regression_data.txt` in the build folder, never in the data folder.

> **Workflow gap:** stage 2 emits `-2-TemMat/`, but stages 3–4 expect
> `-3-Resized/` and `-4-Thershold/`. Both were produced in **ImageJ**,
> not by this codebase: rolling-ball background subtraction (separate
> colours, smoothing disabled) followed by Li-method thresholding, then
> saved under the `-4-Thershold/` naming convention. The per-experiment
> settings are recorded in the `IMAGE PROCESSING` block of
> `experiment_report_data_sheet.csv` in the input data
> (rows 143–149) — but only for the first four experiments; the
> threshold bounds for the remaining 66 were not logged. The four stages
> are therefore not runnable back-to-back, and an exact re-run of the
> thresholding is possible only for experiments 2–4.

**Stages 3 and 4** — read the thresholded sequence produced upstream.
Both take the experiment folder and name as arguments; they differ only
in how the scale factor is supplied. Stage 3 prompts for it on stdin,
stage 4 takes it as a third argument:

```bash
./interface-statistics <experiment_folder> <experiment_name>   # then enter mm-per-pixel when prompted
./final-pipeline       <experiment_folder> <experiment_name> <mm_per_pixel>
```

Note the scale factor here is **millimetres per pixel** — the
reciprocal of the pixels-per-mm value stage 2 asks for.

Expected input layout (`Thershold` is the spelling used by the
upstream folder naming convention):

```
<experiment_folder>/<experiment_name>-4-Thershold/
└── <experiment_name>-Thershold-0001.tif ...
```

Neither stage creates its output image directory, so
`<experiment_folder>/<experiment_name>-5-MinContour/` must exist
beforehand or the contour frames will not be written.

Output: frame statistics, smoothed curvature, FFT (stage 3), B-spline
fit parameters for curvature and amplitude, transition points, and
second derivative — all tab-separated `.txt` alongside the experiment
folder. Stage 4 additionally records its own inputs to
`<experiment_name>-Inputs.txt` for reproducibility.

## Data

The input data are not part of this repository (see *Data availability* in the
[main README](../README.md)). This project uses, relative to `MSC_DATA_DIR`:

- `marker-templates/left.jpg`, `marker-templates/right.jpg` — marker templates for stage 1
- `regression/linear_regression_data.txt` — the archived table, one linear-regression summary row per
  experiment run as stage 2 appended them in 2016; new runs append to the build folder instead

There is no sample stage 2 input here. The only surviving batch table is
the historical seven-column one kept with the batch scripts at
`batch-inputs/stage2_input_table_original.txt` in the input data,
and it is **not** usable as stage 2 input — fed to stage 2 it consumes
the leading sequence-pattern column as the experiment folder and every
subsequent value is skewed. A usable six-column table is generated by
[`../shell/bin/03_build_input_table.sh`](../shell/bin/03_build_input_table.sh).

> **The archived error column is not a standard error.** Stage 2's call
> to `gsl_fit_linear` had its covariance outputs mismatched: GSL returns
> `(c0, c1, cov00, cov01, cov11, sumsq)`, but the code bound `cov01` to
> a variable named `velocity_cov` and `cov11` to one named
> `correlation`. Column 8 of the archived file therefore holds `cov01`,
> the intercept–slope covariance — which is why every value in it is
> negative, and why it scales with the fit window rather than with the
> uncertainty in the velocity.
>
> This is fixed in [`src/02_template_matching.cpp`](src/02_template_matching.cpp),
> which now writes `sqrt(cov11)`, the standard error of the fitted
> velocity. **New rows are not comparable with the archived ones** in
> that column: they will be positive and of a different magnitude. The
> other seven columns are unchanged.
>
> Note also that the surviving source took `sqrt` of the negative
> `cov01`, which yields NaN — so it could not have produced the finite
> values in the archived file. Those were written by an earlier version
> that recorded `cov01` directly.

See [`../shell/README.md`](../shell/README.md) for a comparison of
`regression/linear_regression_data.txt` against its near-duplicate, the shell copy,
including which velocity is correct for the one experiment where they
disagree.
