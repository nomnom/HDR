#!/bin/bash

ddir=data/display/e330

echo "note: on main display, run ./control_display 1366 768 0 --image $ddir/c_marker.png"
echo "press enter to continue"

read
img=tmp/tmp_checkpos.exr

sh capture.sh 2.5 22 ${img%.exr} dontshow

#resize/crop to 640x480
convert $img  -resize 720x480 -crop 640x480+40+0 ${img%.exr}_0.exr



./artoolkit_test  data/camera/canon_parameters_close_640.yml  --multi data/display/e330/c_marker.dat 50 false false tmp/tmp_checkpos_%01d.exr 
