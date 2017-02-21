#!/bin/bash

#
# hdr reconstruction and undistort
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

odir=experiments/$id/$subid

# display dimensions
w=1366
h=768


if [ ! -e "experiments/$id/$subid" ] ; then 
  echo "Error: The ID $id/$subid was not found"
  exit -1
fi;

ch=$odir/checker.cr2
if [ ! -e $ch ] ; then
  echo "Error: checker image $ch not found";
  exit -1
fi

# convert to .exr and undistort

for f in $odir/*.cr2 ; do

  if [ -e ${f%.cr2}_undist.exr ] ; then
    echo "skipping $f, because ${f%.cr2}_undist.exr already exists"
    continue
  fi

  echo "running dcraw on $f";
  dcraw -4 -h $f

  # checker image only requires dcraw
  if [ $f = $ch ] ; then continue; fi

  # check for darkframe
  df=$odir/`echo \`basename $f\` | sed 's/img/df/g'`
  if [ $f = $df ] ; then continue; fi  # darkframe only requires dcraw
  if [ ! -e $df ] ; then 
    echo "Warning: darkframe $df not found; continuing without subtracting darkframe!"
  fi

  f=${f%.cr2}.ppm
  df=${df%.cr2}.ppm
  if [ -e $df ] ; then  
    echo "subtracting darkframe $df"
    ./imgtools -i $f -i $df -S -o $f
  fi 
  
  echo "converting $f to ${f%.ppm}.exr"
  convert $f ${f%.ppm}.exr 
  rm $f
 
  f=${f%.ppm}.exr

  echo "undistorting $f"  
  ./undistort_display - ${ch%.cr2}.ppm $f $w $h ${f%.exr}_undist.exr

  echo 
done

# cleanup ppm files
rm $odir/*.ppm

# HDR reconstruction

home=`pwd`

cd $odir

hdrgen=exps_all_undist.hdrgen
if [ ! -e exps_all_undist.hdrgen ] ; then
  echo "Error: $odir/$hdrgen not found!"
  exit -1
fi


echo "creating HDR image $odir/hdr_undist.exr"
pfsinhdrgen $hdrgen | pfshdrcalibrate --bpp 16 --response-file $home/data/camera/response_canon.m | pfsoutexr hdr_undist.exr


echo "finished"
exit 0
