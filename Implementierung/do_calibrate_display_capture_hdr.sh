#!/bin/bash

#
# capture exposure series of display for hdr reconstruction
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

odir=experiments/$id/$subid

  if [ -e "$odir" ] ; then 
   read -n 1 -p "The ID $id/$subid already exists! Overwrite [y/n]?"
   if [ ! "$REPLY" = "y" ] ; then exit 1; fi
  # rm -r $odir
   echo
  fi;
  mkdir -p $odir

# display dimensions
w=1366
h=768

ap=5

# set display background light 
# $1 : 0 - 100
function bglight() {
  python -c "print int($1 / 100.0 * 4437.0)"  > /sys/class/backlight/intel_backlight/brightness
}


bglight 100

echo "press enter to start capture"
read



# checkerboard 8x6
./control_display $w $h 5 --checker 8 6 &
 sleep 1
 ss=1/8
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss

  gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $odir/checker.cr2

#  dcraw -4 -h $odir/checker.cr2

sleep 1

# exposure series of grey ramp
echo -n "" > $odir/exps_all_undist.hdrgen
echo -n "" > $odir/exps_all.hdrgen

./control_display $w $h 0 --rgb 1 1 1  &
i=0
for ss in 0.5 1/4 1/8; do


  echo "capturing image with ss=$ss and ap=$ap"

 # setup canon 
 echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss

bglight 0; sleep 1
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $odir/df_$i.cr2
bglight 100; sleep 1
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $odir/img_$i.cr2

 # convert from raw to ppm
# ( dcraw -4 -h $odir/img_$i.cr2 ;  convert $odir/img_$i.ppm $odir/img_$i.exr; rm $odir/img_$i.ppm ) &

 # add entry to .hdrgen
 echo img_${i}_undist.exr `echo 1.0 / \( $ss \) | bc -l` 5 100 0 >> $odir/exps_all_undist.hdrgen
 echo img_${i}.exr `echo 1.0 / \( $ss \) | bc -l` 5 100 0 >> $odir/exps_all.hdrgen

 i=`echo 1 + $i | bc`

done

killall -TERM control_display
exit
sleep 10; # wait for dcraw...

# undistort images
for f in $odir/img_*.exr ; do 
 ./undistort_display -  $odir/checker.ppm $f $w $h ${f%.exr}_undist.exr
done


cd $odir 

pfsinhdrgen exps_all_undist.hdrgen | pfshdrcalibrate --bpp 16 --save-response response.m | pfsoutexr hdr_undist.exr

