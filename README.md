# Buckling of a floating viscous layer: MSc analysis code

[![Licence: MIT](https://img.shields.io/badge/licence-MIT-blue.svg)](LICENSE)
[![Tests](https://github.com/amirhs1/viscous-layer-buckling/actions/workflows/tests.yml/badge.svg)](https://github.com/amirhs1/viscous-layer-buckling/actions/workflows/tests.yml)

This repository holds the analysis code from an MSc thesis in physics
(Institute for Advanced Studies in Basic Sciences, IASBS, Zanjan, 2014–2017).

In the experiment, a thin viscous layer floats on a liquid substrate inside a rectangular
boundary, and two opposite sides move together at a constant velocity. Under this
compression the layer first thickens and then buckles. The video of each run shows the
transition. The code turns the videos into amplitude and curvature time series, measures
the thicken-to-buckle transition, and produces the thesis figures.

The thesis text is not included here.

## Layout

| Folder | What it is |
|--------|------------|
| [`opencv/`](opencv/README.md) | The analysis. A C++ pipeline (OpenCV / GSL / Boost) in four sequential stages, built by one CMake project, that extracts amplitude and curvature over time from the video. `variants/` keeps three experimental versions that are not built. |
| [`shell/`](shell/README.md) | Batch drivers. They assemble the per-experiment inputs, run the 70 experiments through the pipeline unattended, and plot the result. |
| [`gnuplot/`](gnuplot/README.md) | The figures. Thirteen gnuplot files produce 21 figures (20 for the thesis and gnuplot's `test` page), and `./render.sh` renders them all. `velocity_frequency/` holds the critical-velocity fit. |
| [`tests/`](tests/README.md) | The test suite. It generates its own inputs, so it needs no data: `tests/run_tests.sh`. |

The folders depend on each other in one direction only. `shell/` drives `opencv/`:
its step 4 feeds the batch table to the stage 2 executable. `gnuplot/` stands alone and
plots from the input data (see *Data layout*).

**One step of the pipeline is not code.** Between OpenCV stages 2 and 3, the frames were
background-subtracted and thresholded by hand in **ImageJ**. No macro was ever saved, so
nothing here reproduces that step, and the four OpenCV stages cannot be run back-to-back.
The ImageJ parameters were logged in the lab data sheet, but only for the first four
experiments. See the workflow-gap note in [`opencv/README.md`](opencv/README.md) and the
`IMAGE PROCESSING` table in [`shell/README.md`](shell/README.md).

The repository keeps only source code. The scripts regenerate every figure from the input
data (the 21 in `gnuplot/`, the two in `gnuplot/velocity_frequency/` and the two from
`shell/plots/`), so no rendered output is stored.

## Requirements

| For | Needs |
|---|---|
| `opencv/` | CMake >= 3.5, a C++11 compiler, OpenCV 4 or later, GSL, Boost `filesystem` (built and tested on 2026-11-14 with CMake 4.5, OpenCV 5.1, GSL 2.8, Boost 1.92) |
| `shell/` | Bash 3.2+, gnuplot 5+ with the `pngcairo` terminal |
| `gnuplot/` | gnuplot 5+ (verified on 6.0); ghostscript only for `render.sh --png` |
| `tests/` | nothing more than `opencv/` and `gnuplot/` need |

## Data availability

The input data are not included in this repository. They are the processed results of the
experiments: the lab data sheet for all 70 runs, the batch inputs, ImageJ length logs,
per-run linear-regression tables, and the smoothed series and transition tables behind the
thesis figures (about 4 MB of text and two small images).

The data are available from the author on reasonable request. To ask for them, open an issue
in this repository and say what you intend to use them for. Shared data are released under
CC BY 4.0.

To run the code, put the data in any folder and set `MSC_DATA` to it; *Data layout* below lists
the subfolders each part of the code reads.

## Data layout

Set `MSC_DATA` to the folder that holds the input data before running anything:

```bash
export MSC_DATA=/path/to/data
```

There is no default: every script stops with a message if `MSC_DATA` is not set. The code
reads these parts of the data folder and never writes to it:

| Path under `MSC_DATA` | Read by |
|---|---|
| `series/<series>/` | the 21 gnuplot figures (`gnuplot/render.sh`) |
| `regression/` | the critical-velocity fit (`gnuplot/velocity_frequency/`) |
| `curvature-reference/` | the two plots in `shell/plots/` |
| `experiment_report_data_sheet.csv` | the batch-table builder (`shell/bin/03_build_input_table.sh`) |
| `marker-templates/` | OpenCV stage 1, located when the project is configured (`cmake -DMSC_DATA_DIR=…`) |

The data folder also keeps records the code does not read: the batch inputs used in 2016
(`batch-inputs/`) and the ImageJ length logs (`length-logs/`). The code never writes to the data
folder. The figures go to `gnuplot/images/`, the lists of shell step 3 to `shell/work/` and stage 2's
regression rows to the CMake build folder, all ignored by git; the OpenCV stages and shell
steps 1–2 write into the experiment folder they are given, as they did in 2016.

## Status

This is archival research code from 2015–2016. It is published as it was used, apart from the
changes needed to build and run it today and the crash fixes listed under *Changes since 2016*
in [`opencv/README.md`](opencv/README.md). Bugs that affect the archived results are kept and
listed there under *Known issues*.

## Licence

The code is released under the MIT licence; see [`LICENSE`](LICENSE). The input data are
not part of this repository; when shared, they are released under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (see *Data availability*). To cite this repository, see
[`CITATION.cff`](CITATION.cff).

The OpenCV stages link the [GNU Scientific Library](https://www.gnu.org/software/gsl/), which is
released under the GPL v3, so binaries built from them are covered by the GPL v3. The B-spline
fits in stages 3 and 4 follow the example in the GSL manual.
