#!/bin/bash

# canon camera control script used by lightstage

log=log/remote_camera.log

echo `date` " -- $* " >> $log

case $1 in

  #initialize canon camera
  setup) 
   gphoto2 --camera "Canon EOS 5D Mark II" --set-config autopoweroff=0  --set-config iso=100 --set-config whitebalance=1 
    exit $?
    ;;
 
  # simple image capture, either on camera or with direct download   
  capture)
    ss=$2
    ap=$3
    file=$4
    if [ -z "$ss" ] || [ -z "$ap" ] || [ -z "$file" ] ; then
      echo "capture: argument missing!" >> $log
      exit -1
    fi

    # 5th arg given: download d
    #if [ "$5" = "download" ] ; then
      gphoto2 --camera "Canon EOS 5D Mark II"  --set-config shutterspeed=$ss --set-config aperture=$ap --capture-image-and-download --filename $file --force-overwrite | tee $log
    #else
    #  gphoto2 --camera "Canon EOS 5D Mark II" --set-config shutterspeed=$ss --set-config aperture=$ap --capture-image  | tee $log 
    #fi
    exit $?
    ;;

  # bulb mode
  bulb)
    
    ;;

  #argument fail
  *) echo "unknown command: $1" >> $log ; exit -1;;
esac 
