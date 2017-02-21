#!/bin/bash

#
# illumination script: demo

amixer sset Master 40000


id=demo
subid=demo

odir=experiments/$id/$subid
log=$odir/lightstage.log

mode=hold
echo "running in mode $mode"

# check if dir exists, if it doesn't create the structure and continue from last state otherwise
if [ -e "$odir"  ] && [ -e "$odir/exposures.log" ]; then

 # get index of last illumination
 index=`tail -n1 $odir/exposures.log | cut -d' ' -f1`
 echo "continuing from last state (index $index)" | tee -a $log
 if [ -n "$1" ] ; then index=$1; fi
else

 #create dir structure
 echo "creating directories..." | tee -a $log
 mkdir -p $odir
 mkdir $odir/envmap_completed 
 mkdir $odir/envmap_remaining
 mkdir $odir/envmap_used 
 mkdir $odir/result
 mkdir $odir/screen
 mkdir $odir/tracking 

fi;




# lenovo 
#cam_config=data/camera/lenovo_parameters.yml
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
