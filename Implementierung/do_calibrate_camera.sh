#!/bin/bash

#
# Single-picture camera calibration script using the OpenCV camera calibiration example. 
#  Takes several images of a checkerboard of known dimensions and dumps the parameters  
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

pout=experiments/$id
if [ -e "experiments/$id" ] ; then 
 read -n 1 -p "The ID $id already exists! Overwrite [y/n]?"
 if [ ! "$REPLY" = "y" ] ; then exit 1; fi
 rm -r $pout
 echo
fi;
mkdir -p $pout

numpics=9
delay=3

mode=0; # 0 = direct capture for video devices
if [ "$2" = "canon" ] ; then mode=1; fi # 1 = canon mode
if [ "$2" = "uvcwebcam" ] ; then mode=2; fi # 2 = canon mode

ss=1/200; if [ -n "$3" ] ; then ss=$3; fi
ap=2.8; if [ -n "$4" ] ; then ap=$4; fi

camid=0
if [ -n "$3" ] ; then camid="$3"; fi 

if [ "$mode" = "1" ] ; then
  echo "capturing image with canon camera every $delay seconds ($numpics pictures total)"
elif [ "$mode" = "2" ] ; then
  echo "capturing image from /dev/video$camid every $delay seconds ($numpics pictures total)"
fi

squaresize_mm=25.5

#
# direct capture mode
#
if [ $mode -eq 0 ] ; then
#  ./calibrate_camera -w 4 -h 11 -n $numpics -d `echo "1000 * $delay" | bc -l` -oe -pt acircles -s 9.1 -o $pout/parameters.yml
  ./calibrate_camera $camid -w 8 -h 6 -n $numpics -d `echo "1000 * $delay" | bc -l`  -pt chessboard -s $squaresize_mm -o $pout/parameters.yml
fi


#
# canon: indirect capture mode (capture images, create xml file, then call calibrate_camera)
#
if [ $mode -eq 1 ] ; then
 echo "using canon camera (aperture is $ap, shutterspeed is $ss)"
 gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss

 for i in `seq $numpics`; do
 # echo "press return for next picture"
 # read

  # capture image
  gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $pout/image_$i.cr2

  # convert to ppm (in background)
  echo "running dcraw"
  dcraw -4 -h $pout/image_$i.cr2 &
  
   sleep $delay
#   convert $pout/image_$i.ppm -resize 720x480 -crop 640x480+40+0 $pout/image_$i.ppm 
 done

 # create xml file
 echo '<?xml version="1.0"?>
 <opencv_storage>
 <images>' > $pout/images.xml
 for i in `seq $numpics`; do
   echo $pout/image_$i.ppm >> $pout/images.xml
 done
 echo '</images>
</opencv_storage>' >> $pout/images.xml

 
  
 # call calibrate_camera
 ./calibrate_camera -w 8 -h 6  -oe -s 24.0 -o $pout/parameters.yml $pout/images.xml 


fi



#
# webcam: indirect capture mode (capture images, create xml file, then call calibrate_camera)
#
if [ $mode -eq 2 ] ; then

  # set camera parameters
  uvcdynctrl -L data/camera/lenovo_webcam.gpfl

  # capture images
  for i in `seq $numpics`; do
    echo "capturing image $i / $numpics"

    guvcview -l data/camera/lenovo_webcam.gpfl --no_display --exit_on_close -i $pout/image_$i.bmp -c 1 -m 1
    convert $pout/image_$i.bmp $pout/image_$i.ppm
    read
  done

fi
