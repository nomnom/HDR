#!/bin/bash

ss=1/200; if [ -n "$1" ] ; then ss=$1; fi
ap=2.8; if [ -n "$2" ] ; then ap=$2; fi

pout=tmp
mkdir -p $pout
fout=$pout/tmp_canon_preview

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
#  rm $fout.cr2
  pfsv $fout.ppm

