#!/bin/bash

# display calibration aquisition script (location-dependent response curve)
# control display (shows several grey shades), captures images, undistort them and extract raw values
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
ap=`grep aperture $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`




#
# image capture
#


# setup canon 
# $1 = ss, $2 = ap
function setup() {
 echo "setting aperture = $2  shutterspeed = $1)"
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

# convert cr2 to exr
function convert_raw() {
  b=${1%.*}
  dcraw -4 -h $b.cr2
  convert $b.ppm $b.exr
}

# time to show images on display in seconds (should be equal or shorter than the 
# time gphoto2 need to capture and download the image, but longer than the shutterspeed)
d=3

# delay between showing display image and camera capture command  in seconds
dt=1.5
echo "please turn off all lights and press enter to start capturing"
read 

sleep 2

sh sound_notification.sh 2

# number of steps (divisible by 4)
num=32

# interval size
size=8

# exposure time (4 intervals)
exp1=`grep exposure_1 $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
exp2=`grep exposure_2 $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
exp3=`grep exposure_3 $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
exp4=`grep exposure_4 $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
exp_dark=`grep exposure_black $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

# longest exposure for darkframe
setup $exp_dark $ap

# 1 darkframe
bglight 0
sleep $dt; capture "1_darkframe";
bglight 100

if [ 0 -eq 1 ] ; then
# shortest exposure for checker 
setup $exp4 $ap

# 2 checkerboard 8x6
./control_display $w $h 0 --checker 8 6 &
sleep $dt ; capture "2_checker"
killall -KILL control_display

# 3 capture image series
for i in `seq -w 0 1 $num`; do

  # check which range we are in
  ival=`echo $i/$size | bc` 
  exp=0
  if [ $ival -eq 0 ] ; then exp=$exp1; 
  elif [ $ival -eq 1 ] ; then exp=$exp2;
  elif [ $ival -eq 2 ] ; then exp=$exp3; 
  else  exp=$exp4; fi
  
  setup "$exp" $ap
 
  # pixel value; map 0..255 -> 0..1
  # steps are: 0 7 15 ... 255
  if [ $i -eq 0 ] ; then 
    v=0
  else
    v=`echo "($i*$size - 1)/255.0" | bc -l` 
  fi
#  echo "pixel value is " `echo $v \* 255 |  bc -l`
  # capture uniform screen:
  ./control_display $w $h 0 --rgb $v $v $v &
  sleep $dt ; capture "3_grey_$i"
  killall -KILL control_display
 
done

fi

# 4 capture lowest pixel value 
 setup $exp_dark $ap
 v=`echo 1.0/255.0 |bc -l`
 ./control_display $w $h 0 --rgb $v $v $v &
 sleep $dt ; capture "4_lowest$i"
 killall -KILL control_display


sh sound_notification.sh 6 

echo "finished capturing, you can turn the lights back on"

echo "to process the images, run do_calibrate_display_all.sh"
