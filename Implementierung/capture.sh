#!/bin/bash

ss=1/200; if [ -n "$1" ] ; then ss=$1; fi
ap=2.8; if [ -n "$2" ] ; then ap=$2; fi

 
if [ -z "$3" ] ; then
  fout=tmp/tmp_canon_preview
else 
  fout=$3
fi

# set default config
gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss

# download image like this: 
gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $fout.cr2
  echo "running dcraw"
  dcraw -4 -h $fout.cr2
  convert $fout.ppm -flip -flop $fout.exr

# if custom output file is given, dont delete the cr2 file, and also convert toe exr image 
if [ -z "$3" ] ; then
  rm $fout.cr2
fi 

# if 4th argument is set, dont preview image
if [ -z "$4" ] ; then
  pfsv $fout.exr
fi
