set size 1,1
set origin 0,0

unset label

set multiplot

set size 1, 0.166
set origin 0,0
plot "e.txt" using 1 with linespoint

set size 1, 0.166
set origin 0,0.166
plot "a.txt" using 1 with linespoint

set size 1, 0.166
set origin 0,0.333
plot "d.txt" using 1 with linespoint

set size 1, 0.166
set origin 0,0.5
plot "g.txt" using 1 with linespoint

set size 1, 0.166
set origin 0,0.666
plot "b.txt" using 1 with linespoint

set size 1, 0.166
set origin 0,0.833
plot "high_e.txt" using 1 with linespoint

unset multiplot
