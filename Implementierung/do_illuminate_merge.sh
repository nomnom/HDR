#!/bin/bash

#
# postprocess illumination captures
#

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID"; exit 1; fi


explog=$3
if [ -z "$explog" ]; then explog=exposures.log; fi

outfile=$4
if [ -z "$outfile" ]; then outfile=result.exr; fi

minval=$5
if [ -z "$minval" ]; then minval=0.0 ; fi





odir=experiments/$id/$subid
dir=$odir/result
response=data/camera/response_canon.m

# cat crop and flip
crop=" -crop 1600x1500+2150+950 "
flip=" -flip -flop "

# eval overlap
#crop=" -crop 1600x1500+2000+1000 "
#flip=""

#crop=""
# check if dir exists and get number of illuminations
if [ -e "$odir"  ] ; then

 # get index of last illumination
 maxIdx=`tail -n1 $odir/$explog | cut -d' ' -f1`

else
 echo "directory $odir does not exist"
 exit -1
fi

echo "processing `echo $maxIdx + 1 | bc` illuminations..."

for i in `seq 0 1 $maxIdx`; do




 # processed result goes here:
 res=$dir/${i}_res.exr 
 
 # check if everythin is there
 if [ ! -e "$dir/$i.cr2" ] ; then 
   echo "file not found: $dir/$i.cr2"
   exit -1
 fi
 if [ ! -e "$dir/${i}_df.cr2" ] ; then 
   echo "file not found: $dir/${i}_df.cr2"
   exit -1
 fi

 # darkframe
 if [ ! -e "$dir/${i}_df.exr" ] ; then
   if [ ! -e  "$dir/${i}_df.ppm" ] ; then 
     dcraw -4  $dir/${i}_df.cr2 
   fi
   convert $dir/${i}_df.ppm  $crop  $flip  $dir/${i}_df.exr 
  rm  $dir/${i}_df.ppm
 
 fi

 # illumination
 if [ ! -e "$dir/$i.exr" ] ; then
   if [ ! -e "$dir/$i.ppm" ] ; then
     dcraw -4  $dir/$i.cr2 
   fi
   convert $dir/$i.ppm  $crop $flip $dir/$i.exr 
  rm  $dir/${i}.ppm
 fi
 
 if [ ! -e "$res" ] ; then
   # subtract darkframe
   ./imgtools2 -i $dir/$i.exr -i $dir/${i}_df.exr -S -o $res

   # apply response curve
   ./imgtools2 -i $res -r $response  -p "762 481" -p "85 1224" -o $res

   #also fixes deadpixels

 else
   echo "$res already exists; skipping. "
 fi

done

echo "postprocessing complete. creating result ..."


#
# remove double lines in exposures logfile due to forced repeats after successful illumination
#

tmpfile=$odir/$explog.fixed 
echo -n > $tmpfile
for i in `seq 0 1 $maxIdx`; do
   # take latest occurance
   grep -e "^$i\\s" $odir/$explog  | tail -n1 >> $tmpfile 
done


#
# combine images to result
# 

./reconstruct $odir $explog.fixed $odir/$outfile.exr $minval

#for i in `seq 0 1 $maxIdx`; do
## merge envmap_used
#arg=""
#  
#
# f=$dir/../envmap_used/$i
#   if [ ! -e "$f.png" ] ; then
#     echo "downscaling envmap_used no. $i..." 
#     convert $f.exr -resize 1200x200 $f.png 
#   fi
#  arg="$arg -i $f.png "
#
#done
#./imgtools2  $arg -A -m 0.2 -o $odir/${outfile%.exr}_used.png


echo "done!" 
