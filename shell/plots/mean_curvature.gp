#!/usr/bin/env gnuplot
#
# Mean interface curvature against normalized frame number, for one or
# more experiments.
#
# Input: any tab-separated file written by stage 3/4 of the OpenCV
# pipeline whose columns are
#   1: frame number   2: normalized frame number   3: smoothed curvature (1/mm)
# That is the layout of "<experiment_name>-SmoothedData.txt", and of the
# reference files spline.txt and moving_average.txt in $MSC_DATA/curvature-reference/.
#
# Defaults plot the two reference smoothings of the same experiment, so
# the script runs with no arguments. Override either series and the
# output path on the command line:
#
#   gnuplot -e "series_a='A-SmoothedData.txt'; \
#               label_a='4.1 Hz'; \
#               series_b='B-SmoothedData.txt'; \
#               label_b='8.8 Hz'" plots/mean_curvature.gp

# Input data folder: DATA_DIR if given (render.sh passes it), otherwise $MSC_DATA.
if (!exists("DATA_DIR")) DATA_DIR = system('printf "%s" "$MSC_DATA"')
if ((!exists("series_a") || !exists("series_b")) && DATA_DIR eq "") { exit error "MSC_DATA is not set: export MSC_DATA=/path/to/data (see Data availability in README.md)" }
if (!exists("series_a")) series_a = DATA_DIR."/curvature-reference/spline.txt"
if (!exists("label_a"))  label_a  = "B-spline smoothing"
if (!exists("series_b")) series_b = DATA_DIR."/curvature-reference/moving_average.txt"
if (!exists("label_b"))  label_b  = "Moving-average smoothing"
if (!exists("outfile"))  outfile  = "mean_curvature.eps"

set terminal postscript eps color colortext size 5in,3.5in
set output outfile

set xlabel "Normalized frame number"
set ylabel "Mean curvature (mm^{-1})"
set title  "Mean interface curvature"

set key top right
set grid

# Column 1 is the raw frame index; plot against the normalized one.
plot series_a using 2:3 with lines lw 2 title label_a, \
     series_b using 2:3 with lines lw 2 title label_b

unset output
