#!/bin/bash

projects="calibrate_camera control_display undistort_display evaluate_display show_on_display artoolkit_test generate_patterns lightstage reconstruct"
here=`pwd`
for p in $projects; do
  cd src/$p ; make ; cd $here
done
