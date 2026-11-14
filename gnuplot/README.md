# Gnuplot examples — MSc, buckling of a floating viscous layer

Thirteen gnuplot files producing 21 figures. Everything reads from the `series/`
folder of the input data, which `MSC_DATA` names (see *Data layout* in the
[main README](../README.md)), and writes to `images/`.

Part of [this repository](../README.md), alongside the
[OpenCV pipeline](../opencv/README.md) and its
[batch drivers](../shell/README.md).

The rendered figures are **not** bundled — they are regenerated from the data
in seconds, so only the scripts are kept here. `render.sh`
creates `images/` on first run.

## Rendering

```bash
export MSC_DATA=/path/to/data   # the input data folder; there is no default
./render.sh              # render all 21 figures
./render.sh 04 09        # render only figures starting 04 / 09
./render.sh --png        # also convert EPS/PS to PNG (needs ghostscript)
./render.sh --clean      # delete generated images
```

Verified with gnuplot 6.0 — all 21 render without warnings. A single script can also be
run by hand from this folder once `images/` exists (`mkdir -p images; gnuplot 02-curve-fit.txt`);
it reads `MSC_DATA` itself.

## Structure

| Path          | What it is                                             |
| ------------- | ------------------------------------------------------ |
| `overlay.gp`  | One parameterized script producing 9 of the 21 figures |
| `figures.tsv` | Config table — one row per overlay figure              |
| `meta/`       | Per-series legend text and transition-frame indices    |
| `NN-*.txt`    | 12 standalone scripts, one per distinct technique      |
| `images/`     | Rendered output — created by `render.sh`, not bundled  |

### `overlay.gp` — the parameterized one

Six scripts differed **only** in which file they read, which column they
plotted, and what they were named. They are now one script driven by
`figures.tsv`:

```bash
gnuplot -e "DATA='$MSC_DATA/series/l175_h2/smooth.txt'; COL=9; META='l175_h2'; \
            OUT='images/variance.eps'; YLAB='Variance (mm^{2})'" overlay.gp
```

Per-run legend text and transition markers live in `meta/`, keyed by series, so
adding a figure means adding a row to `figures.tsv` — not copying a script.

### Standalone scripts

| #   | Script                      | Technique                                                       |
| --- | --------------------------- | --------------------------------------------------------------- |
| 01  | run-diagnostics-multiplot   | 3-panel A / K / sigma, shared x-axis, annotated transition      |
| 02  | curve-fit                   | nonlinear `fit` + `stats`, fitted curve over raw data           |
| 03  | strain-vs-time-presentation | multi-series, pattern fills, dashed critical-strain markers     |
| 06  | local-strain                | `stats` prefixes, computed columns, grouped by layer thickness  |
| 07  | period-variance             | derived quantity from multi-column data                         |
| 08  | strain-curvature            | as 06, but scaled by _instantaneous_ rather than initial length |
| 10  | single-run-plot             | log-log comparison of B-spline vs moving-average smoothing      |
| 11  | multiplot                   | landscape multi-panel layout                                    |
| 12  | movave-curvature-4data      | four explicit data files on shared axes                         |
| 13  | movave-curvature-forloop    | `plot for [IDX=0:10]` with `columnheader` titles                |
| 16  | movave-curvature-multiplot  | 10-panel grid, each comparing run 0 against run _i_             |
| 19  | color-palette-helper        | shared colour/style definitions                                 |

06 and 08 were deliberately **not** merged: they use different physics (initial
vs time-dependent length scaling), not different formatting.

## Data

`$MSC_DATA/series/` keeps the original per-series subfolders, which is what disambiguates
otherwise-colliding names — the working tree had three _different_ files called
`smooth.txt`, plus two different `statis.txt`, `transition.txt` and
`period-data.txt`. (`Statis.txt` and `statis.txt` would also collide on a
case-insensitive filesystem.)

| Folder                                          | Series                            |
| ----------------------------------------------- | --------------------------------- |
| `l175_h2.85/`                                   | L = 175 mm, h = 2.85 mm — 11 runs |
| `l175_h2/`                                      | L = 175 mm, h = 2 mm — 12 runs    |
| `l200_h2.5/`, `all/`                            | L = 200 mm, h = 2.5 mm — 5 runs   |
| `transition/`, `transition_final_presentation/` | 18-column summary tables          |
| `D1394_*/`                                      | per-run working folders           |

Column meanings (from the header of any `smooth-*.txt`):

```
3 = A moving-average   4 = K moving-average
5 = A B-spline         7 = K B-spline      9 = variance moving-average
```

**"Spline" vs "moving average" is a column choice, not a gnuplot setting.** No
script here calls gnuplot's `smooth`; both were computed upstream in the
OpenCV/C/C++ pipeline. Script 10 plots the two against each other.

The two smoothing methods also yield _different_ detected transition frames, so
`meta/` keeps separate marker sets per method (`_spl` vs `_var`).

## Notes from curation

- 31 scripts reduced to 13 files, while figure count rose 19 → 21: the extra
  two came from originals that had no curated counterpart and are now rows
  `04b` / `05b` in `figures.tsv`.
- Several scripts previously wrote to the **same** output name and silently
  overwrote each other in a batch run; every figure now has a unique name.
- Fixed legacy syntax rejected by gnuplot 6:
  `set terminal postscript eps ... landscape`, and `w p color black`.
- Scripts 10 and 16 had no `title`, so gnuplot printed the raw **file path**
  into the legend; both now carry proper labels.
- Removed a block pasted from gnuplot's own `stats` documentation, which
  referenced `orbital_elements.dat` — a gnuplot demo file never part of this
  project — and used a mismatched `stats` prefix.
- Deduplicated the data: 11 byte-identical copies removed. Files that merely
  _looked_ redundant were kept, notably the per-run `TransitionPoint.txt`
  files, which are a richer 8-column revision of the 6-column series copies.
