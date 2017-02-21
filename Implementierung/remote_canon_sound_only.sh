#!/bin/bash

# dummy canon camera control script used by lightstage


case $1 in

  # simple image capture, either on camera or with direct download   
  capture)
    ss=$2
    ap=$3
    file=$4
    if [ -z "$ss" ] || [ -z "$ap" ] || [ -z "$file" ] ; then
      echo "capture: argument missing!"
    fi
    aplay data/sounds/shutter.wav &
    sleep $ss
    exit $?
    ;;

  setup)
    exit 0 ;;


  #argument fail
  *) echo "unknown command: $1" ; exit -1;;
esac 

exit 0
