#!/bin/bash

# runs several required steps at once
id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

conf=$3
if [ -z "$conf" ]; then echo "Please specify the display config file (e.g. data/mbp.config) with the third argument"; exit 1; fi



odir=experiments/$id/$subid



# if no target exposure is specified, use that of the darkframe
exp=`grep exposure_target $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

#if [ -z "$4" ] ; then
#  df=$odir/1_darkframe.cr2
#  if [ ! -e $df ] ; then df=$odir/1_darkframe_1.cr2; fi
#  if [ -e $df ] ; then 
#    exp=`exiftool $df | egrep "^Exposure Time" | cut -d ':' -f2 | sed 's/ //g'`
#  else 
#    echo "Error: darkframe $odir/1_darkframe.cr2 or $odir/1_darkframe_1.cr2 not found; please specify the target exposure time with the third argument"
#    exit -1
#  fi
#fi

echo "target exposure is $exp"

# average darkframe, black/whitescreen iamges
need_avg=`find $odir | grep -ce '.*_.*_[[:digit:]]\.cr2'`
if [ $need_avg -gt 0 ] ; then
   echo "averaging multiple exposed images"
   bash do_calibrate_display_average.sh $1 $2 $exp
fi

#subtract darkframe and undistort images
bash do_calibrate_display_undistort.sh $1 $2 $3 $exp

# get inverse response curve
bash get_response_svr.sh $1 $2 $3

