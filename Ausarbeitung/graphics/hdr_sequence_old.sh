#~/bin/bash

# create dynamic range images and heatmaps

source ./config

size='400,300'
drdir=$idir/experiments/lenovo/dr2
num=3

# graph target directory
graphfile="`pwd`/hdr_beleuchtung/lenovo_hdr_"
response=$idir/data/camera/response_canon_linear.m

# everything runs in the impl. directory

cd $drdir;

for f in *.cr2; do
 if [ ! -e ${f%.cr2}.exr ] ; then 
  if [ ! -e ${f%.cr2}.ppm] ; then 
   dcraw -4 -h $f
  fi 
  convert $f ${f%.ppm}.exr; 
 fi
done

ss=20
cd $idir;
for f in  '3_black' '4_white' '5_lowest' ; do 
 f=$drdir/$f.exr
 if [   -e  "$f" ] ; then 
  
  # create averages
  args=""
  for i in `seq $num`; do
    args="$args -i ${f%.exr}_$i.exr"
  done
 echo $args
  ./it $args -A -m `echo 1.0 / $num | bc -l`  -o $f
 fi

 
 #undistort
 o=${f%.exr}_undist.exr 
 if [  -e $o ] ; then 
   ./undistort_display data/camera/canon_parameters_close.yml $drdir/2_checker.exr $f 1366 768  $o
 fi
done


 # dynamic range calc
 ./it -i $drdir/4_white_undist.exr -i $drdir/3_black_undist.exr -g 13 -S -c"0 1" -o $drdir/maxvals_noblack.exr
 ./it -i $drdir/5_lowest_undist.exr -i $drdir/3_black_undist.exr -g 13 -S -c "0 1" -o $drdir/minvals_noblack.exr
 
 
