#!/bin/bash

#
# illumination script: single shot mode
#

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID"; exit 1; fi

odir=experiments/$id/$subid
log=$odir/lighstage.log

mode=single
if [ -n "$3" ]; then mode=$3; fi

echo "running in mode $mode"

# check if dir exists, if it doesn't create the structure and continue from last state otherwise
if [ -e "$odir"  ] && [ -e "$odir/exposures.log" ]; then

 # get index of last illumination
 index=`tail -n1 $odir/exposures.log | cut -d' ' -f1`
 echo "continuing from last state (index $index)" | tee -a $log

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
cam_config=data/camera/lenovo_parameters.yml
disp_config=data/display/e330/parameters.yml 
stage_config=data/lightstage_default.yml

#
#run illumination
#

# start new run
if [ -z "$index" ] ; then 
  ./lightstage 0 $cam_config $disp_config $stage_config $odir $mode | tee -a $log

# continue from last run
else
  ./lightstage 0 $cam_config $disp_config $stage_config $odir $mode $index | tee -a $log
fi
