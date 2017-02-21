#!/bin/sh

# average cube edge length in mm
size=149
# half size for translate
t=`echo $size / 2.0 | bc -l`

# 2) single sides for rotation evaluation (every orthogonal pair of panels has the same orientation)
#
 

# 1x1
outf=data/marker/testcube_1x1.dat
./generate_patterns --panel 59 1 1 80 400 200 data/marker t -$t   0 -$t  y 90 z 180
./generate_patterns --panel 58 1 1 80 400 200 data/marker t   0 -$t -$t  x -90 z 90 
echo 2 > $outf
for m in 1x1_58 1x1_59 ; do
  tail -n+8 data/marker/panel_${m}_2.dat >> $outf
done


# 2x2
outf=data/marker/testcube_2x2.dat
./generate_patterns --panel 0 2 2 48 400 200 data/marker t -$t   0 -$t  x -90 y 90
./generate_patterns --panel 4 2 2 48 400 200 data/marker t   0 -$t -$t  x -90 
echo 8 > $outf
for m in 2x2_0 2x2_4 ; do
  tail -n+8 data/marker/panel_${m}_2.dat >> $outf
done

# 3x3
outf=data/marker/testcube_3x3.dat
./generate_patterns --panel 8 3 3 30 400 200 data/marker t  0 -$t -$t   x -90 z 90
./generate_patterns --panel 17 3 3 30 400 200 data/marker t -$t   0 -$t   y 90 z 180 
echo 18 > $outf
for m in 3x3_8 3x3_17 ; do
  tail -n+8 data/marker/panel_${m}_2.dat >> $outf
done


