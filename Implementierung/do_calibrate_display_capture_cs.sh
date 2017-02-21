#!/bin/bash

#
# control display, capture red/green/blue screen and calculate color transformation matrix
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

echo "capturing image with ss=$ss and ap=$ap"

#
# image capture
#


# setup canon 
# $1 = ss, $2 = ap
function setup() {
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss
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

# time to show images on display in seconds (should be equal or shorter than the 
# time gphoto2 need to capture and download the image, but longer than the shutterspeed)
d=3.5

# delay between showing display image and camera capture command  in seconds
dt=1.5
echo "please turn off all lights and press enter to start capturing"
read 

sleep 2

setup $ss $ap

# 1: darkframe
bglight 0
for i in `seq 5`; do
  sleep $dt; capture "1_darkframe_$i";
done 
bglight 100

# 2: checkerboard 8x6
./control_display $w $h $d --checker 8 6 &
sleep $dt ; capture "2_checker"

# 3: uniform red
./control_display $w $h  $d --rgb 1 0 0 &
sleep $dt ; capture "3_red"

# 4: uniform green
./control_display $w $h $d --rgb 0 1 0 &
sleep $dt ; capture "4_green"

# 5: uniform blue
./control_display $w $h $d --rgb 0 0 1 &
sleep $dt ; capture "5_blue"

echo "finished capturing, you can turn the lights back on"

echo "undistorting screen images"
sh do_calibrate_display_all.sh $1 $2 $3

echo "calculating color space transformation matrix"
./evaluate_display --color $odir/{3_red,4_green,5_blue}.exr $odir/cs_transform.yml

