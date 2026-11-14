# Changelog

All notable changes to this repository are documented here. The format is
[Common Changelog](https://common-changelog.org/), with one departure: the 1.0.0 section lists the
changes made to the code as used in 2015–2016 instead of only "Initial release". Bugs that affect
the archived results are kept and listed under *Known issues* in
[`opencv/README.md`](opencv/README.md). This project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1](https://github.com/amirhs1/viscous-layer-buckling/releases/tag/v1.0.1) - 2026-11-14

### Added

- Add the Zenodo DOI to `CITATION.cff` and the README, with a badge (`b0b4e22`)
- Add the author's ORCID iD to `CITATION.cff` (`599d20b`)

## [1.0.0](https://github.com/amirhs1/viscous-layer-buckling/releases/tag/v1.0.0) - 2026-11-14

_Initial release_ of the analysis code as used in 2015–2016: the OpenCV pipeline, the shell batch
drivers and the gnuplot figure scripts (`03cd997`, `88b38aa`, `d051141`), with the changes below.

### Changed

- Build and run with OpenCV 4 and later: use `RETR_EXTERNAL`, `CHAIN_APPROX_NONE` and `FILLED`, which replace the C constants OpenCV 4 removed (`dcba118`)
- Read every input from the folder that `MSC_DATA` names, which has no default, and write only to ignored folders: `gnuplot/images/`, `shell/work/` and the CMake build folder (`03cd997`, `88b38aa`, `d051141`, `c4bfc22`, `f2d7748`)
- Replace the MATLAB table extraction, `extract_data.m`, with `extract_data.gp`, which writes the same columns in the same number format (`cc43339`)
- Name figures 05 and 15 after the variance they plot: `05-variance-vs-time-l200` and `15-movave-variance-index` (`069082f`)

### Added

- Add `MSC_NO_DISPLAY`, which runs the stages without preview windows (`80268bb`)
- Add a test suite that generates its own inputs, so it needs no data, and run it with GitHub Actions on every push (`01020cb`, `9033601`, `6dbc27f`)
- Add the README, with the data availability statement, and *Changes since 2016* and *Known issues* in `opencv/README.md` (`a2e0fff`, `5d4c88d`, `79f3ce0`, `cdf26c5`)
- Add the MIT licence and the citation metadata, with a licence badge (`657123b`, `32d69bf`, `49ce77d`)

### Removed

- Remove 121 lines of commented-out code from the four stages (`15d73d5`)
- Remove the gnuplot scripts' `Source` comments, which named 2016 files that no longer exist, and their shebang lines, which had no effect (`f0c34b4`)

### Fixed

- Fix a use after free in stage 4: free the derivative array after its last use, not inside `BSplineCurvature` (`80268bb`)
- Stop stages 3 and 4 with a message at an empty contour or a missing original frame instead of reading out of bounds; require stage 4's fourth argument, and report "no period" when there are fewer than two extrema (`80268bb`)
- Read thresholded frames correctly with OpenCV 4.6, which ignored the request to keep them single-channel and aborted, and with colour files, which OpenCV 5 misread: convert each frame to one channel before `findContours` (`dcba118`, `6dbc27f`)
- Read stage 2's folder and name as whole lines, so paths may contain spaces, and free per-frame arrays instead of leaking them (`80268bb`)
- Print row counts in shell step 3 without the padding macOS `wc` adds (`d5f711d`)
- Spell "Normalized" in figure text and correct the "Tansition" typo in the title of figure 06 (`dab0b02`)
