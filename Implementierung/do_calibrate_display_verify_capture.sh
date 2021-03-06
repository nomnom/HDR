#!/bin/bash

#
# show equalized images and take images, verify result
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
echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
gphoto2 --camera "Canon EOS 5D Mark II" \
       --set-config iso=100 \
       --set-config whitebalance=1 \
       --set-config aperture=$ap \
       --set-config shutterspeed=$ss

# canon capture function as shortcut
# image will be saved under $odir
# $1 = filename
function capture () {
echo  gphoto2 --camera "Canon EOS 5D Mark II" \
          --capture-image-and-download --force-overwrite --filename $odir/$1.cr2
}


# set display background light 
# $1 : 0 - 100
function bglight() {
  sh set_backlight.sh $1
}

# time to show images on display in seconds (should be equal or shorter than the 
# time gphoto2 need to capture and download the image, but longer than the shutterspeed)
d=3.5

# delay between showing display image and camera capture command  in seconds
dt=1.5
echo "please turn off all lights and press enter to start capturing"
read 


# display image with desired relative radiance
function show() {
  ./show_on_display --image $caldir/$1 $caldir/r_white.exr $caldir/r_black.exr 0 $caldir/response_lut.yml 15 15 $d &
  #./show_on_display --image_svr $caldir/$1 $caldir/response_svr.exr 23 24 $caldir/response_svr_p30.yml $d &
}
sleep 2


# 1: darkframe = including black screen
bglight 100
./control_display $w $h $d --rgb 0 0 0 &
sleep $dt; capture "1_darkframe";

# 2: checkerboard 8x6 (uncalibrated)
./control_display $w $h $d --checker 8 6 &
sleep $dt ; capture "2_checker"

# 3: uniform white
show "c_white.exr"
sleep $dt ; capture "3_white"

# 4: 2x horizontal ramp 
show "c_ramp_h2.exr" 
sleep $dt ; capture "4_ramp_h2"

# 5: lowfreq rgb noise
show "c_noise2.exr" 
sleep $dt ; capture "5_noise2"


echo "finished capturing, you can turn the lights back on"

echo "to process the images, run do_calibrate_display_all.sh"

