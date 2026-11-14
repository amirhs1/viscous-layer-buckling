# extract_data.gp - build the frequency-velocity table that plot.txt fits.
#
# Reads the per-experiment linear-regression table that OpenCV stage 2 writes
# (8 tab-separated columns, see opencv/README.md) and writes four columns:
#
#   Frequency        column 6, the motor frequency (Hz)
#   Frequency_error  0.2 Hz for every experiment
#   Velocity         minus column 7, the fitted compression velocity (mm/s)
#   Velocity_error   sqrt(-column 8); column 8 is cov01, not a standard
#                    error (see the note in opencv/README.md)
#
# This is a port of the 2016 MATLAB script extract_data.m, with the same
# columns and the same number format (15 significant digits).
#
# Run from this folder:     gnuplot extract_data.gp
# Other input or output:    gnuplot -e "INPUT='table.txt'; OUTPUT='out.txt'" extract_data.gp
# Defaults: INPUT  = $MSC_DATA/regression/linear_regression_data.txt
#           OUTPUT = ../images/Velocity-frequency.txt

if (!exists("DATA_DIR")) DATA_DIR = system('printf "%s" "$MSC_DATA"')
if (!exists("INPUT") && DATA_DIR eq "") { exit error "MSC_DATA is not set: export MSC_DATA=/path/to/data (see Data availability in README.md)" }
if (!exists("INPUT"))  INPUT  = DATA_DIR."/regression/linear_regression_data.txt"
if (!exists("OUTPUT")) { system "mkdir -p ../images"; OUTPUT = "../images/Velocity-frequency.txt" }

set datafile separator "\t"
set table $ROWS
plot INPUT using (sprintf("%.15g\t%.15g\t%.15g\t%.15g", $6, 0.2, -$7, sqrt(-$8))) with table
unset table

set print OUTPUT
print "Frequency\tFrequency_error\tVelocity\tVelocity_error"
do for [i = 1:|$ROWS|] { if (strlen($ROWS[i]) > 0) { print $ROWS[i] } }
unset print
