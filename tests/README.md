# Tests

The tests need no real data. `make_inputs.cpp` (an OpenCV program, built by `CMakeLists.txt`
here) generates every input, chosen so that the right answer is known in advance, and
`run_tests.sh` runs the code on them and checks the results.

```bash
tests/run_tests.sh
```

The suite needs nothing beyond what the repository already needs: CMake, a C++ compiler, OpenCV,
GSL, Boost, gnuplot and Bash (`awk` does the arithmetic). Everything is written under `tests/work/`
(ignored by git), which each run starts afresh. A run takes under a minute, mostly building.
GitHub Actions runs it on every push (`.github/workflows/tests.yml`: Ubuntu 24.04, OpenCV 4.6); it
was also run with OpenCV 5.1 on macOS.

## What is checked

| Part | Input | Check |
|---|---|---|
| build | two made-up marker templates | the input generator and the four stages build; configuring without `MSC_DATA_DIR` stops with a message |
| stage 1 | the markers in a 640 x 240 sequence | reads it and prints the frame size |
| stage 2 | the markers moving 2 px per frame each, 60 fps, 6.3 px per mm | velocity −38.0952 mm/s (−4 × 60 / 6.3) and scale 1/6.3 mm per pixel |
| stage 3 | a sinusoidal interface whose amplitude grows 1 px per frame | amplitude and variance of every frame (0.1 mm per pixel); stops with a message at a frame with no contour |
| stage 4 | the same interface, period switch 0 | amplitude and variance of every frame; writes the smoothed data and the transition point |
| stage 4 | an amplitude oscillating with a period of 24 frames, switch 1 | reports 25.7143 frames, the value listed under *Known issues* in `opencv/README.md` |
| stage 4 | three arguments only | prints its usage and exits non-zero |
| shell steps 1–4 | a made-up experiment tree and data sheet (Excel line endings) | renaming (dry run, apply, refusal to overwrite); stage folders; the batch table, its column order and its refusal of empty cells or a row-count mismatch; the batch run, its failure count and the values passed to stage 2 |
| shell steps 3–4 + stage 2 | the stage 2 experiment, run as a batch | a second regression row with the same velocity |
| gnuplot | a two-row regression table | `extract_data.gp` writes frequency, 0.2, −velocity, √(−error); it and `render.sh` stop with a message without `MSC_DATA` |

Not covered: the 21 figure scripts themselves (they plot the real data), and the three programs in
`opencv/variants/`, which are not built.
