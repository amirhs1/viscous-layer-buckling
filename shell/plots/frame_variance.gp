#!/usr/bin/env gnuplot
#
# Per-frame variance of the interface profile against normalized frame
# number -- the amplitude growth curve of the buckling instability.
#
# Input: "<experiment_name>-FrameStatistics.txt" from stage 3 of the
# OpenCV pipeline (stage 4 writes other columns), or the reference copy. Columns are
#   1: frame number      2: normalized frame number  3: maximum y (mm)
#   4: minimum y (mm)    5: abs(max/min)             6: frame variance (mm^2)
#   7: mean curvature    8: peak FFT frequency
#
# With no arguments it plots the reference copy in $MSC_DATA/curvature-reference/;
# override with:
#
#   gnuplot -e "series='A-FrameStatistics.txt'; outfile='A.png'" \
#           plots/frame_variance.gp

# Input data folder: DATA_DIR if given with -e, otherwise $MSC_DATA.
if (!exists("DATA_DIR")) DATA_DIR = system('printf "%s" "$MSC_DATA"')
if (!exists("series") && DATA_DIR eq "") { exit error "MSC_DATA is not set: export MSC_DATA=/path/to/data (see Data availability in README.md)" }
if (!exists("series"))  series  = DATA_DIR."/curvature-reference/frame_statistics.txt"
if (!exists("outfile")) outfile = "frame_variance.png"

set terminal pngcairo size 900,600 enhanced
set output outfile

set xlabel "Normalized frame number"
set ylabel "Frame variance (mm^2)"
set title  "Interface profile variance over time"

set key off
set grid

plot series using 2:6 with lines lw 2 lc rgb "#1f77b4"

unset output
