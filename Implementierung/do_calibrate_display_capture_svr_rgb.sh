#!/bin/bash

# display calibration aquisition script (location-dependent response curve)
# control display (shows several grey shades), captures images, undistort them and extract raw values
# 


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

odir=experiments/$id/$subid

# display dimensions
w=1366
h=768


ss="1/8"; if [ -n "$3" ] ; then ss="$3"; fi
ap="5"; if [ -n "$4" ] ; then ap="$4"; fi


  if [ -e "experiments/$id/$subid" ] ; then 
   read -n 1 -p "The ID $id/$subid already exists! Overwrite [y/n]?"
   if [ ! "$REPLY" = "y" ] ; then exit 1; fi
  # rm -r $odir
   echo
  fi;
  mkdir -p $odir



#
# image capture
#


# setup canon 
# $1 = ss, $2 = ap
function setup() {
 ss="1/8"; if [ -n "$1" ] ; then ss="$1"; fi
 ap="5"; if [ -n "$2" ] ; then ap="$2"; fi
 echo "setting aperture = $ap  shutterspeed = $ss)"
 gphoto2 --camera "Canon EOS 5D Mark II" \
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

has_bgcontrol=0
if [ -e /sys/class/backlight/intel_backlight/brightness ] ; then
  has_bgcontrol=1
fi

# set display background light 
# $1 : 0 - 100
function bglight() {
if [ $has_bgcontrol -eq 1 ] ; then 
  python -c "print int($1 / 100.0 * 4437.0)"  > /sys/class/backlight/intel_backlight/brightness
fi
}

# convert cr2 to exr
function convert_raw() {
  b=${1%.cr2}
  dcraw $b.cr2
  convert $b.ppm $b.exr
}

# time to show images on display in seconds (should be equal or shorter than the 
# time gphoto2 need to capture and download the image, but longer than the shutterspeed)
d=3.5

# delay between showing display image and camera capture command  in seconds
dt=1.5
echo "please turn off all lights and press enter to start capturing"
read 

if [ $has_bgcontrol -eq 0 ] ; then
 echo "no background light control available - please turn off the display, press enter, wait for the image capture and then turn it back on again"
 read
fi

sleep 20

#aperture
ap=5

# number of steps (divisible by 4)
num=32

# exposure time (4 intervals)
exp1=0.5
exp2=1/2
exp3=1/4
exp4=1/8

setup $exp4 $ap

# 1 darkframe
bglight 0
sleep $dt; capture "1_darkframe";
bglight 100

# 2 checkerboard 8x6
./control_display $w $h $d --checker 8 6 &
sleep $dt ; capture "2_checker"

# 3 capture image series
for i in `seq -w $num`; do

  # check which range we are in
  ival=`echo \($i-1\)/8 | bc` 
  exp=0
  if [ $ival -eq 0 ] ; then exp=$exp1;
  elif [ $ival -eq 1 ] ; then exp=$exp2;
  elif [ $ival -eq 2 ] ; then exp=$exp3;
  else  exp=$exp4; fi
  
  setup "$exp" 5
  
  # capture uniform screen:
  v=`echo $i/$num | bc -l` 
  ./control_display $w $h $d --rgb $v 0 0  &
  sleep $dt ; capture "3_red_$i"
  ./control_display $w $h $d --rgb 0 $v 0 &
  sleep $dt ; capture "3_green_$i"
  ./control_display $w $h $d --rgb 0 0 $v &
  sleep $dt ; capture "3_blue_$i"
 
done

echo "finished capturing, you can turn the lights back on"




