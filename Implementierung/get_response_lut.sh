#!/bin/bash

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

odir=experiments/$id/$subid

if [ ! -e "$odir" ] ; then
  echo "Error: The ID $id/$subid was not found"
  exit -1
fi;

border=15
area=0.8
ramp=$odir/5_ramp_h2_undist.exr
white=$odir/4_white_undist.exr
black=$odir/3_black_undist.exr
numramps=2

tmpfile=tmp/tmp_plot_$$.dat

# calculate response curve points from recorded screen images
./evaluate_display --response $ramp $white $black $area $border $numramps 4 $odir/response_lut.yml  #>/dev/null 2>/dev/null

if [ ! $? -eq 0 ] ; then 
  echo "Error running evaluate_display"
  exit -1
fi

if [ ! "$1" = "noplot" ] ; then
   ./evaluate_display --response $ramp $white $black $area $border $numramps 3 $tmpfile  >/dev/null 2>/dev/null
 echo " plot '$tmpfile' u 1:2 w l,'$tmpfile' u 1:3 w l,'$tmpfile' u 1:4 w l, (x/`echo -n``wc -l $tmpfile | cut -d' ' -f1`` `)**(1.0/2.2), 1.055*(x/`echo -n``wc -l $tmpfile | cut -d' ' -f1`` `)**(1/2.4)-0.055" | gnuplot -p
fi 



