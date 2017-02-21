#!/bin/bash

#
# calculate averages of darkframe / whitescreen / blackscreen images
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

odir=experiments/$id/$subid


if [ ! -e "experiments/$id/$subid" ] ; then 
  echo "Error: The ID $id/$subid was not found"
  exit -1
fi


# experiment: calculate white screen by merging red/green/blue screen
#num=2
#for pre in 4_red 4_green 4_blue; do
#  for i in `seq $num` ; do
#    of=$odir/${pre}_$i
#    if [ ! -e $of.cr2 ]; then echo "note: $of.cr2 missing" ; exit -1; fi
#    if [ ! -e $of.exr ] ; then
#      if [ ! -e $of.ppm ] ; then 
#        echo "converting $of.cr2 to $of.ppm"
#        dcraw -4 -h $of.cr2; 
#      fi
#      echo "converting $of.ppm to $of.exr"
#      convert $of.ppm $of.exr
#      rm -f $of.ppm
#    fi
#
#
#  done
#  ./imgtools2 `for i in \`seq $num\` ; do echo -n "-i $odir/${pre}_$i.exr "; done` -v -o $odir/$pre.exr
#
#  for i in `seq $num` ; do
#    rm $odir/${pre}_$i.exr
#  done
#done
#
## merge channels 
#./imgtools2 -i $odir/4_red.exr -i $odir/4_green.exr -i $odir/4_blue.exr -C -o $odir/4_white.exr 
#
## ramp images
#for c in red green blue; do
#  ( dcraw -4 -h $odir/5_ramp_h2_$c.cr2 ; convert $odir/5_ramp_h2_$c.ppm $odir/5_ramp_h2_$c.exr)&
#done
#./imgtools2 -i $odir/5_ramp_h2_red.exr -i $odir/5_ramp_h2_green.exr -i $odir/5_ramp_h2_blue.exr -C -o $odir/5_ramp_h2.exr 
#
#exit

# convert all images to exr and then calculate average with imgtools; fix exposure if desired
for pre in 1_darkframe 3_black 4_white; do
  for i in `seq 5` ; do
    of=$odir/${pre}_$i
    if [ ! -e $of.cr2 ]; then echo "error: $of.cr2 missing" ; exit -1; fi
    if [ ! -e $of.exr ] ; then
      if [ ! -e $of.ppm ] ; then 
        echo "converting $of.cr2 to $of.ppm"
        dcraw -4 -h $of.cr2; 
      fi
      echo "converting $of.ppm to $of.exr"
      convert $of.ppm $of.exr
      rm -f $of.ppm
    fi

    # fix exposure
    if [ -n "$3" ] ; then
      exp=$3
      echo "performing exposure equalization to $exp as requested"
    
      # original exposure time   
      exp_from=`exiftool $of.cr2 | egrep "^Exposure Time" | cut -d ':' -f2 | sed 's/ //g'`
 
      if [ ! "$exp_from" = "$exp" ] ; then
        echo "changing exposure of $of.exr from $exp_from to $exp"
       ./imgtools2 -i $of.exr -m `echo "($exp) / ($exp_from)" | bc -l` -o $of.exr
      fi
    fi

  done
  ./imgtools2 `for i in \`seq 5\` ; do echo -n "-i $odir/${pre}_$i.exr "; done` -v -o $odir/$pre.exr

 # for i in `seq 5` ; do
 #    rm $odir/${pre}_$i.exr
 # done
done


