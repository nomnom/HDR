#!/bin/bash

#
# capture dynamic range of screen: 1) blackscreen 2) checker 3) whitescreen 4) lowest pixel value > 0
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

ap=`grep aperture $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

ss_dark=20
ss_white=2.5
echo "capturing image with ss=$ss and ap=$ap"

#
# image capture
#


# setup canon 
# $1 = ss, $2 = ap
function setup() {
 echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
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
# $1 : brightness (0 - 100)
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

bglight 100
setup $ss_white $ap

# 2: checkerboard 8x6
./control_display $w $h 0 --checker 8 6 &
sleep $dt ; capture "2_checker"
killall -KILL control_display

# 4: uniform highest value
v=1.0
./control_display $w $h 0 --rgb $v $v $v  &
for i in `seq 3`; do
 sleep $dt ; capture "4_white_$i"
done
killall -KILL control_display



setup $ss_dark $ap

# 1: darkframe
bglight 0
./control_display $w $h 0  --rgb 0 0 0 &
for i in `seq 3`; do
  sleep $dt; capture "1_darkframe_$i";
done 
killall -KILL control_display
bglight 100

# 3: uniform black
./control_display $w $h 0  --rgb 0 0 0 &
for i in `seq 3`; do
 sleep $dt ; capture "3_black_$i"
done
killall -KILL control_display

# 4: uniform lowest value
v=`echo 1.0/255.0 | bc -l`
./control_display $w $h 0 --rgb $v $v $v  &
for i in `seq 3`; do
 sleep $dt ; capture "5_lowest_$i"
done
killall -KILL control_display

echo "finished capturing, you can turn the lights back on"

echo "to process the images, run do_calibrate_display_all.sh"
