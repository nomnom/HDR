#!/bin/bash

bf=data/display/e330/r_black.exr
wf=data/display/e330/r_white.exr

cp experiments/lenovo/0/5_ramp_white_undist.exr tmp/rh1.exr
cp experiments/lenovo/hdr_ramp_h1i/img_2_undist.exr  tmp/rh1i.exr
#
cp experiments/lenovo/hdr_ramp_v2/img_2_undist.exr  tmp/rv2.exr
cp experiments/lenovo/hdr_ramp_v2i/img_2_undist.exr  tmp/rv2i.exr
#
cp experiments/lenovo/hdr_ramp_h3/img_2_undist.exr  tmp/rh3.exr
cp experiments/lenovo/hdr_ramp_h3i/img_2_undist.exr  tmp/rh3i.exr

wfd=tmp/wf-d.exr
./imgtools2 -i $wf -i $bf -S -o $wfd 
#sub, div
for f in h1 h1i h3 h3i v2 v2i; do
  ./imgtools2 -i tmp/r$f.exr -i $bf -S -o tmp/r${f}-d.exr 
  ./imgtools2 -i tmp/r${f}-d.exr -i $wfd -D -o tmp/r${f}-e.exr 
  convert tmp/r${f}-e.exr -crop 1365x768+0+0 tmp/${f}-e.exr
done


##flop
#for f in h1i h3i; do
#  convert tmp/r${f}-e.exr -flop tmp/r${f}-e.exr
#done
##flip
#for f in v2i; do
#  convert tmp/r${f}-e.exr -flip tmp/r${f}-e.exr
#done

#sum
for f in h1 h3 v2; do
  ./imgtools2 -i tmp/r${f}i-e.exr -i tmp/r$f-e.exr -A -o tmp/sum_$f.exr 
done
