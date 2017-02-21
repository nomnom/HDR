#!/bin/bash

#
# undistort images (subtract darkframe if present)
#


id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

conf=$3
if [ -z "$conf" ]; then echo "Please specify the display config file (e.g. data/mbp.config) with the third argument"; exit 1; fi

odir=experiments/$id/$subid

if [ ! -e "experiments/$id/$subid" ] ; then 
  echo "Error: The ID $id/$subid was not found"
  exit -1
fi

# display dimensions
w=`grep display_width $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
h=`grep display_height $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`

exp=$4
if [ -z "$4" ]; then 
 exp=`grep exposure_target $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
fi

# camera parameters
cparams=`grep camera_parameters $conf | cut -d ' ' -f 3 | sed -s 's/\s\+//g'`
if [ -z "$cparams" ] ; then cparams=-; fi

# 1) process files: dcraw to exr
for f in $odir/*.cr2 ; do
#  if [[ `basename $f` =~ .*_.*_.?\.cr2 ]] ; then
#    echo "excluding pre-averaged image $f"
#    continue
#  fi
  f=${f%.cr2}
  if [ ! -e ${f}_raw.exr ] ; then 
    if [ ! -e $f.ppm ] ; then 
      echo "running dcraw on $f.cr2"
      dcraw -4 -h $f.cr2
    fi  
    echo "converting $f.ppm to ${f}_raw.exr"
    convert $f.ppm ${f}_raw.exr
  fi
done


df=$odir/1_darkframe_raw.exr
cf=$odir/2_checker_raw.exr
response=data/camera/response_canon_linear.m

#if [ 0 -eq 1 ] ; then

# with canon build-in noise reduction order is reversed

# 1) map over camera response curve
for f in $odir/*_raw.exr; do
  if [[ $f = $df ]] ; then
    continue
  fi
  echo "applying response curve $response to $f"
  ./imgtools2 -i $f -r $response -o ${f%_raw.exr}.exr
done



# 2) if requested with third arg: equalize exposure time of all images
if [ -n "$exp" ] ; then
  echo "performing exposure equalization to $exp as requested"

  for f in $odir/*_raw.exr; do
    if [ $f = $df ] ; then continue; fi
 
    f=${f%_raw.exr}.exr

     # original exposure time   
    if [ -e ${f%.exr}.cr2 ] ; then
      exp_from=`exiftool ${f%.exr}.cr2 | egrep "^Target Exposure Time" | cut -d ':' -f2 | sed 's/ //g'`
     
       # evaluate fractions:
       #exp=`echo $exp | bc -l`
       #exp_from=`echo $exp_from | bc -l`
       if [ ! "$exp_from" = "$exp" ] ; then 
         echo "changing exposure of $f from $exp_from to $exp"
        ./imgtools2 -i $f -m `echo "($exp) / ($exp_from)" | bc -l` -o $f
       fi
    else
      echo "Note: ${f%.exr}.cr2 not found; performing no exposure equalization (default for averaged images)"
    fi
  done
fi

# 3) subtract background illumination from all images
if [ ! -e $df ] ; then 
  echo "Note: darkframe $df not found! Continuing without subtracting darkframe!"
fi
for f in $odir/*_raw.exr; do
  if [ $f = $df ] ; then continue; fi
  f=${f%_raw.exr}.exr 
  echo "subtracting darkframe $df from $f"
  ./imgtools2 -i $f -i $df -S -c "0 1" -o $f
done

#fi;


# 4) undistort all images except darkframe
if [ ! -e $cf ] ; then
   echo "Error: checker $cf was not found! Exiting."
   exit -1
fi
for f in $odir/*_raw.exr ; do
  
  # no undistort for darkframe
  if [ $f = $df ] ; then continue; fi 
  f=${f%_raw.exr}.exr
  
  
  echo "undistorting $f"  
  ./undistort_display $cparams $odir/2_checker.exr $f $w $h ${f%.exr}_undist.exr
  
done 


## 5) cleanup
##cho "cleaning up..."
##or f in $odir/*.exr ; do
 # if [[ `basename $f` =~ .*_undist.exr ]] ; then
 #   continue
 # fi
 # rm -v ${f%.exr}.ppm
 # if [[ $f =~ .*1_darkframe.exr ]] || [[ $f =~ .*3_black.exr ]] || [[ $f =~ .*4_white.exr ]]; then
 #   continue
 # else 
 #   rm -v $f
 # fi
#done

echo "finished"
exit 0


