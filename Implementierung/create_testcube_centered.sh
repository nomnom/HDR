#!/bin/sh

# testcube with origin at the center

# average cube edge length in mm
size=149
# half size for translate
t=`echo $size / 2.0 | bc -l`

#
# 1) whole cube (testcube.dat)
#

# 1x1
./generate_patterns --panel 58 1 1 80 400 200 data/marker t 0 $t 0  x 90
./generate_patterns --panel 59 1 1 80 400 200 data/marker t  0  0  $t 

# 2x2
./generate_patterns --panel 0 2 2 48 400 200 data/marker t -$t   0 0 x -90 y 90
./generate_patterns --panel 4 2 2 48 400 200 data/marker t   0 -$t 0 x -90 

# 3x3
./generate_patterns --panel 8 3 3 30 400 200 data/marker t   $t  0 0  z -90 x -90
./generate_patterns --panel 17 3 3 30 400 200 data/marker t   0  0 -$t  z 90 y 180  


outf=data/marker/testcube_centered.dat

# concatenate panels to valid multimarker data
echo 28 > $outf
for m in 2x2_0 2x2_4 3x3_8 3x3_17 1x1_58 1x1_59 ; do
  tail -n+8 data/marker/panel_${m}_2.dat >> $outf
done

