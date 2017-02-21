#!/bin/bash

#
# show hdr sequence of exponential ramp
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

conf=$3
if [ -z "$conf" ]; then echo "Please specify the display config file (e.g. data/mbp.config) with the third argument"; exit 1; fi


odir=experiments/$id/$subid

if [ -e "experiments/$id/$subid" ] ; then 
 read -n 1 -p "The ID $id/$subid already exists! Overwrite [y/n]?"
 if [ ! "$REPLY" = "y" ] ; then exit 1; fi
# rm -r $odir
 echo
fi;
mkdir -p $odir


# display dimensions
w=`grep display_width $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
h=`grep display_height $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

ss=`grep exposure_max $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
ss_black=`grep exposure_black $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
ap=`grep aperture $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

caldir=`grep caldir $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`



#
# image capture
#

# setup canon 
function setup() {
 echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$2 \
        --set-config shutterspeed=$1
}

# canon capture function as shortcut
# image will be saved under $odir
# $1 = filename
function capture () {
  gphoto2 --camera "Canon EOS 5D Mark II" \
          --capture-image-and-download --force-overwrite --filename $odir/$1.cr2
}


# set display background light 
# $1 : 0 - 100
function bglight() {
  sh set_backlight.sh $1
}

# delay between showing display image and camera capture command  in seconds
dt=1.5

echo "please turn off all lights and press enter to start capturing"
read 


# display image with desired relative radiance
function show() {
 # ./show_on_display --image $caldir/$1 $caldir/r_white.exr $caldir/r_black.exr 0 $caldir/response_lut.yml 15 15 $d &
  #./show_on_display --image_svr $caldir/$1 $caldir/response_svr.exr 23 24 $caldir/response_svr_p30.yml $d &
 echo "============ " $1 " ============"  >> $odir/show_on_display.log
 ./show_on_display --hdr_sequence $2 $3 $caldir/$1.exr $caldir/parameters.yml $4 $5 $6 >> $odir/show_on_display.log   
}

sleep 1
sh sound_notification.sh 2
sleep 1 

ss_hdr=15
setup $ss_hdr $ap

# 1: darkframe = including black screen
bglight 100
./control_display $w $h 0 --rgb 0 0 0 &
sleep $dt; capture "1_darkframe";
killall -KILL control_display

setup $ss $ap

# 2: checkerboard 8x6 (uncalibrated)
./control_display $w $h 0 --checker 8 6 &
sleep $dt ; capture "2_checker"
killall -KILL control_display


numframes=100
for ss_hdr in 6 10 15; do

fps=`echo $numframes/$ss_hdr | bc -l` 

# 3: expramp 3 magnitudes
show "c_eramp3" $numframes $fps "3_eramp3_${ss_hdr}.cr2" $ss_hdr $ap
killall -KILL show_on_display

# 3: eramp 5 magnitudes
show "c_eramp5" $numframes $fps  "3_eramp5_${ss_hdr}.cr2" $ss_hdr $ap
killall -KILL show_on_display

done;

echo "finished capturing, you can turn the lights back on"
sh sound_notification.sh 6

echo "to process the images, run do_calibrate_display_all.sh $1 $2 $3"

