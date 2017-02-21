#!/bin/bash

#
# DOT hdr illumination 
#

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID"; exit 1; fi

odir=experiments/$id/$subid

postproc=0
if [ -n "$3" ] ; then
  postproc=1
fi


# lenovo 
cam_config=data/camera/lenovo_parameters.yml
disp_config=data/display/e330/parameters.yml 
#stage_config=data/lightstage_debug.yml

#dot=c_green_dot_500.exr
#dot=c_green_dot_500.exr
dot=c_green_dot_100.exr
# fixed fps (no hdr)
#fps=20
#t=8
#size=`echo $t*$fps | bc -l`

# fixed size for hdr capture
size=160

echo size is $size


ap=2.8
#ss=10

if [ "$postproc" = "0" ] ; then

sh set_backlight.sh 100
 for ss in 10 20 30; do 

  t=`echo $ss - 2 | bc` 
  fps=`echo $size/$t | bc -l` 
  echo "ss = $ss : fps = $fps ; t = $t"

  
  #output prefix
  f=$odir/${dot%.exr}_$ss
  
  
  #darkframe
  ./control_display 1366 768 0 --rgb 0 0 0 &
  sleep 1.5
  sh capture.sh $ss $ap ${f}_df noshow
  killall -KILL control_display
  
  ./show_on_display  --hdr_sequence $size $fps data/display/e330/$dot $disp_config $f.cr2  $ss $ap
  done

else

#crop=" -crop 1800x980+1900+1200 "
crop=" -crop 1600x1000+2050+1550 "
 for ss in 10 20 30; do 
   
  #output prefix
  f=$odir/${dot%.exr}_$ss
 
  if [ ! -e $f.ppm ] ; then 
   dcraw  -4 $f.cr2 
  fi
  if [ ! -e ${f}_df.ppm ] ; then
   dcraw  -4 ${f}_df.cr2 
  fi

   convert $f.ppm $crop -flip -flop $f.exr
   convert ${f}_df.ppm $crop -flip -flop ${f}_df.exr
#   rm $f.ppm ${f}_df.ppm
  
  #./it -i $f.exr -i ${f}_df.exr -S -c "0 1" -p "762 481" -p "85 1224"  -o ${f}_res.exr
  ./it -i $f.exr -i ${f}_df.exr -S -c "0 1"   -o ${f}_res.exr

done
fi

#pfsv ${f}_res.exr
