#!/bin/bash

#
# control display, capture images, undistort them and extract raw values
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

  echo "capturing image with ss=$ss and ap=$ap"


#
# image capture
#


# setup canon 
# $1 = ss, $2 = ap
function setup() {
ss="1/8"; if [ -n "$1" ] ; then ss="$1"; fi
ap="5"; if [ -n "$2" ] ; then ap="$2"; fi
 echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
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

sleep 2

setup "1/8" 5

# 1: darkframe
bglight 0
for i in `seq 5`; do
  sleep $dt; capture "1_darkframe_$i";
done 
bglight 100

if [ $has_bgcontrol -eq 0 ] ; then
 echo "now press enter"
 read
 sleep 2
fi;

# 2: checkerboard 8x6
./control_display $w $h $d --checker 8 6 &
sleep $dt ; capture "2_checker"
setup 4 5
# 3: uniform black
for i in `seq 5`; do
./control_display $w $h `echo $d+4|bc` --rgb 0 0 0 &
sleep $dt ; capture "3_black_$i"
done
setup "1/8" 5
# 4: uniform red/green/blue
for i in `seq 2`; do
./control_display $w $h $d --rgb 1 0 0  &
sleep $dt ; capture "4_red_$i"
./control_display $w $h $d --rgb 0 1 0  &
sleep $dt ; capture "4_green_$i"
./control_display $w $h $d --rgb 0 0 1  &
sleep $dt ; capture "4_blue_$i"

done

# 5: 2x horizontal ramp r/g/b 
./control_display $w $h $d --hramp 2 15 1 0 0 &
sleep $dt ; capture "5_ramp_h2_red"
./control_display $w $h $d --hramp 2 15 0 1 0 &
sleep $dt ; capture "5_ramp_h2_green"
./control_display $w $h $d --hramp 2 15 0 0 1 &
sleep $dt ; capture "5_ramp_h2_blue"



echo "finished capturing, you can turn the lights back on"

