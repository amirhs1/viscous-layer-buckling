# Critical velocity against motor frequency

This folder holds a linear fit of the compressing velocity V_c against the motor
frequency f, with residuals. It reads the `regression/` folder of the input data, which
`MSC_DATA` names (see *Data layout* in the [main README](../../README.md)).

- `extract_data.m` builds `Velocity-frequency.txt` from `regression/linear_regression_data.txt`
  and writes it as `../images/data_ok.txt`.
- `plot.txt` fits `V_c = a f` with x and y errors to `regression/Velocity-frequency.txt` and
  writes `velocity-frequency.eps` and `velocity-frequency-residuals.eps` to `../images/`.

Run it with `gnuplot plot.txt` from this folder, with `MSC_DATA` set.
`regression/Wrong-Linear_Regression_Data.txt` in the data folder is an earlier table that was
superseded; it is kept for the record.
