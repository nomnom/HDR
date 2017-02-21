#!/bin/sh

#
# dump all five marker panels for our stage and create the final marker.dat
# stage consists of a cube of edglength 150mm (4x4 marker) in the center of a plane of 500x500 mm (8x 4 marker)

outf=data/marker/stage.dat

# approximated cube edge length (horizontal plane) in mm (varies due to manufacturing)
sx=152.9
sy=153.3
sz=150

# half size for translate
tx=`echo $sx / 2.0 | bc -l`
ty=`echo $sy / 2.0 | bc -l`
tz=`echo $sz / 2.0 | bc -l`


# marker z coordinate is exactly 150mm/2  
t2=75

#
# 1) whole cube (stage.dat)
#

# four sides of the cube
# -X
./generate_patterns --panel 0 2 2 30 400 600 data/marker t -$tx   0 -$tz  x -90 y 90
# -Y
./generate_patterns --panel 4 2 2 30 400 600 data/marker t   0 -$ty -$tz  x -90 
# X
./generate_patterns --panel 8 2 2 30 400 600 data/marker t   $tx  0 -$tz  z -90 x -90
# Y
./generate_patterns --panel 12 2 2 30 400 600 data/marker t 0 $ty -$tz  x 90

# bottom plane (one large panel, center four marker are occluded by the cube)
./generate_patterns --panel 16 6 6 30 400 600 data/marker t 0 0 -$sz 



# concatenate panels to valid multimarker data
echo 52 > $outf
for m in 2x2_0 2x2_4 2x2_8 2x2_12 6x6_16 ; do
  tail -n+8 data/marker/panel_${m}_1.dat >> $outf
done
