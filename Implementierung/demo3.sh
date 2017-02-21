#!/bin/bash

#
# illumination script: demo

amixer sset Master 000


id=demo
subid=demo

odir=experiments/$id/$subid
log=$odir/lightstage.log

mode=show
echo "running in mode $mode"

 #create dir structure
 echo "creating directories..." | tee -a $log
 mkdir -p $odir
 mkdir $odir/envmap_completed 
 mkdir $odir/envmap_remaining
 mkdir $odir/envmap_used 
 mkdir $odir/result
 mkdir $odir/screen
 mkdir $odir/tracking 



# lenovo 
cam_config=data/camera/logitech_hd_parameters.yml
disp_config=data/display/e330/parameters.yml 
stage_config=data/lightstage_demo.yml

echo "using camera config $cam_config" | tee -a $log
echo "using display config $disp_config" | tee -a $log
echo "using lighstage config $stage_config" | tee -a $log

echo "------------- START --------------" | tee -a $log
#
# move mousecursor to bottom right position
#
xdotool mousemove 10000 10000

#
#run illumination
#

cam=1

# start new run
if [ -z "$index" ] ; then 
  ./lightstage $cam $cam_config $disp_config $stage_config $odir $mode | tee -a $log

# continue from last run
else
  ./lightstage $cam $cam_config $disp_config $stage_config $odir $mode $index | tee -a $log
fi
