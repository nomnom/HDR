#!/bin/bash

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

conf=$3
if [ -z "$conf" ]; then echo "Please specify the display config file (e.g. data/mbp.config) with the third argument"; exit 1; fi

odir=experiments/$id/$subid

if [ ! -e "$odir" ] ; then
  echo "Error: The ID $id/$subid was not found"
  exit -1
fi;


#patchconfig
bw=`grep border_width $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
bh=`grep border_height $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
patchsize=`grep patchsize $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`


#number of images minus 1
num=32
if [ -n "$4" ] ; then num=$4; fi
echo "using $num images"
# calculate response curve points from recorded screen images
if [ ! -e $odir/cs_transform.yml ] ; then 

   echo "Warning: colorspace transform $odir/cs_transform.yml not found. Ommitting transform in calculation"; 

    ./evaluate_display --svr $bw $bh $patchsize 0 $odir/response_svr.yml  - `for i in \`seq -w 0 1 $num\`; do echo -n " $odir/3_grey_${i}_undist.exr " ; done` 
 #for i in `seq -w 0 1 $num`; do echo -n " $odir/3_grey_${i}_undist.exr " ; done 
else 
    ./evaluate_display --svr $bw $bh $patchsize 0 $odir/response_svr.yml  $odir/cs_transform.yml `for i in \`seq -w 0 1 $num\`; do echo -n " $odir/3_grey_${i}_undist.exr " ; done` 
fi
if [ ! $? -eq 0 ] ; then 
  echo "Error running evaluate_display"
  exit -1
fi




