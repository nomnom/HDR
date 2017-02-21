#!/bin/bash

# demo script for meeting w/ martin 10.3.2014

ldir=experiments/lenovo

### display calibration

 ## angular dependency TN-display
 if [ -z "$1" ] || [ "$1" = "a" ] ; then 
   for d in v-10 v-5 0 v+5 v+10 ; do
     if [ ! -e $ldir/$d/9_colors_undist.exr ] ; then 
       sh do_calibrate_display_undistort.sh lenovo $d 1/8
     fi
   done
   pfsv  $ldir/{v-10,v-5,0,v+5,v+10}/9_colors_undist.exr
 fi
 
 ## plot display  bg illumination white/darkframe
 if [ -z "$1" ] || [ "$1" = "b" ] ; then 
   for c in r g b; do 
     ./imgplot.sh $ldir/final_2/3_black_undist.exr hmap2d $c 5 0.0 0.005  
     ./imgplot.sh $ldir/final_2/4_white_undist.exr hmap2d $c 5 0.0 0.8 
   done
 fi

 ## single response curve
 if [ -z "$1" ] || [ "$1" = "c" ] ; then 
  sh get_response_lut.sh lenovo final_2
 
 ## SVR: plot two response curves 
  for id in 550 367 990; do
    ./evaluate_display --svr 23 24 30 $id  tmp/tmp_svr_$id.dat `for i in \`seq -w 32\`; do echo -n " $ldir/final_2/3_grey_${i}_undist.exr"; done` 
    gnuplot -p -e "plot 'tmp/tmp_svr_$id.dat' u 1:2 w l ,'tmp/tmp_svr_$id.dat' u 1:3 w l ,'tmp/tmp_svr_$id.dat' u 1:4 w l'"
  done
  gnuplot -p -e "plot 'tmp/tmp_svr_550.dat' u 1:4 w l ,'tmp/tmp_svr_367.dat' u 1:4 w l ,'tmp/tmp_svr_990.dat' u 1:4 w l'"
 fi 

## SVR vs lut : show equalized white screen

 if [ -z "$1" ] || [ "$1" = "d" ] ; then 
   # slice plot
  for f in lut svr_p10 svr_p30; do
    ./imgplot.sh tmp/w_$f.exr h a 950
    ./imgplot.sh tmp/w_$f.exr hmap2d b 5 0.0 0.8
  done
 fi

### marker tracking

 ## tracking demo
 if [ -z "$1" ] || [ "$1" = "e" ] ; then 
   ./artoolkit_test date/camera/lenovo_parameters.yml 100 --multi data/marker/testcube.dat 100 false false
 fi
 
 ## envmap show demo
 if [ -z "$1" ] || [ "$1" = "f" ] ; then 
    ./lightstage 0 data/camera/lenovo_parameters.yml data/marker/testcube.dat 100 data/display/e330/parameters.yml 683 384 data/envmap/ennis.exr 1.5 0  
 fi
 
 ## envmap HDR demo
 if [ -z "$1" ] || [ "$1" = "g" ] ; then 
    ./lightstage 0 data/camera/lenovo_parameters.yml data/marker/testcube.dat 100 data/display/e330/parameters.yml 683 384 data/envmap/ennis.exr 3 1
 fi
  

### 

