# overlay.gp - N-run overlay of one measured quantity, with transition markers.
#
# Replaces six near-identical scripts that differed only in which file they
# read, which column they plotted, and what they were named. Per-run legend
# text and transition-frame indices live in meta/, not in the script.
#
# Required:  DATA  path to an indexed multi-block data file
#            COL   column to plot against normalized time (column 2)
#            META  basename under meta/ supplying .titles/.marks1/.marks2
#            OUT   output path
#            YLAB  y-axis label
# Optional:  TTL   plot title      (default: none)
#            KEY   key placement   (default: "top left")
#            LOG   set to 1 for log-log axes
#
# Data columns (see the header of any smooth-*.txt):
#   3 = A moving-average   4 = K moving-average
#   5 = A B-spline         7 = K B-spline        9 = variance moving-average

if (!exists("DATA") || !exists("COL") || !exists("META") || !exists("OUT") || !exists("YLAB")) {
    print "overlay.gp: need DATA, COL, META, OUT, YLAB"
    exit
}
if (!exists("TTL")) { TTL = "" }
if (!exists("KEY")) { KEY = "top left" }
if (!exists("LOG")) { LOG = 0 }

TFILE = "meta/".META.".titles"
M1    = "meta/".META.".marks1"
M2    = "meta/".META.".marks2"

# Number of runs = number of legend entries recorded for this series.
# tr strips the padding wc emits, which int() will not accept.
COUNT(f) = int(system(sprintf("wc -l < '%s' | tr -d ' '", f)))
NB = COUNT(TFILE)
N1 = COUNT(M1)
N2 = COUNT(M2)

# nth line of a metadata file, as a string / integer
TITLE(i) = system(sprintf("sed -n '%dp' '%s'", i + 1, TFILE))
MARK1(i) = int(system(sprintf("sed -n '%dp' '%s'", i + 1, M1)))
MARK2(i) = int(system(sprintf("sed -n '%dp' '%s'", i + 1, M2)))

set terminal postscript eps colortext color "Courier, 10"
set output OUT
set grid
set xlabel 'Normalized time'
set ylabel YLAB
if (TTL ne "") { set title TTL }
eval "set key ".KEY
set key font "Courier, 10"
if (LOG) { set logscale xy }

# One curve per run, then the transition marker(s) for each run on top.
# Marker sets are optional, so the command is assembled and only the pieces
# that actually have data are included - an empty "for" range would still
# evaluate its MARK() call and fail on the blank line.
CMD = sprintf("plot for [i=0:%d] '%s' index i using 2:%d title TITLE(i)", \
              NB - 1, DATA, COL)
if (N1 > 0) {
    CMD = CMD.sprintf(", for [i=0:%d] '%s' index i using 2:%d \
                        every ::MARK1(i)::MARK1(i) notitle lt 5 lc rgb 'black'", \
                      N1 - 1, DATA, COL)
}
if (N2 > 0) {
    CMD = CMD.sprintf(", for [i=0:%d] '%s' index i using 2:%d \
                        every ::MARK2(i)::MARK2(i) notitle lt 7 lc rgb 'black'", \
                      N2 - 1, DATA, COL)
}
eval CMD

unset output
