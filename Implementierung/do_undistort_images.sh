#!/bin/bash

#
# undistort images (subtract darkframe if present)
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

df=$odir/1_darkframe.cr2
dcraw -4 -h $df
if [ ! -e $df ] ; then 
  echo "Warning: darkframe $df not found; continuing without subtracting darkframe!"
fi

# process files

for f in $odir/*.cr2 ; do
 
  # for  darkframe only dcraw is required
  if [ $f = $df ] ; then continue; fi 
  
#  if [ ! -e ${f%.cr2}.ppm ] ; then 
   echo "running dcraw on $f";
   dcraw -4 -h $f; 
#  fi



  f=${f%.cr2}.ppm
  if [ -e $df ] ; then  
    echo "subtracting darkframe $df"
    ./imgtools -i $f -i ${df%.cr2}.ppm -S -o $f
  fi 
  
  echo "converting $f to ${f%.ppm}.exr"
  convert $f ${f%.ppm}.exr 
  rm $f
 
  f=${f%.ppm}.exr

  echo "undistorting $f"  
  ./undistort_display - $odir/2_checker.exr $f $w $h ${f%.exr}_undist.exr

  echo 
done

echo "finished"
exit 0


