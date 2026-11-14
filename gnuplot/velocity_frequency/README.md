# Critical velocity against motor frequency

This folder holds a linear fit of the compressing velocity V_c against the motor
frequency f, with residuals. It reads the `regression/` folder of the input data, which
`MSC_DATA` names (see *Data layout* in the [main README](../../README.md)).

- `extract_data.gp` builds the table `Velocity-frequency.txt` from
  `regression/linear_regression_data.txt` and writes it to `../images/`. It replaces the 2016
  MATLAB script that built the archived `regression/Velocity-frequency.txt`.
- `plot.txt` fits `V_c = a f` with x and y errors to `regression/Velocity-frequency.txt` and
  writes `velocity-frequency.eps` and `velocity-frequency-residuals.eps` to `../images/`.

Run them from this folder, with `MSC_DATA` set: `gnuplot extract_data.gp`, `gnuplot plot.txt`.
`plot.txt` reads the archived table, so its figures match the thesis. That table holds the same 70
rows as the one `extract_data.gp` writes, in a different order.
`regression/Wrong-Linear_Regression_Data.txt` in the data folder is an earlier table that was
superseded; it is kept for the record.
