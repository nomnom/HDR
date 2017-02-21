#!/bin/bash

ss=1/10; if [ -n "$1" ] ; then ss=$1; fi
ap=5; if [ -n "$2" ] ; then ap=$2; fi

pout=tmp/s4_angle
mkdir -p $pout

# set default config
gphoto2 --camera "Canon EOS 5D Mark II" \
        --set-config autopoweroff=0 \
        --set-config iso=100 \
        --set-config whitebalance=1 \
        --set-config aperture=$ap \
        --set-config shutterspeed=$ss

for i in `seq 6` ; do 
fout=$pout/img_$i
# download image like this: 
gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $fout.cr2
  echo "running dcraw"
  dcraw -4 -h $fout.cr2
#  rm $fout.cr2

sleep 2;
done
