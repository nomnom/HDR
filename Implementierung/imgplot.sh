#!/bin/bash

if [ $# = 0 ] ; then 
  echo -e "Usage: $0 <image> <mode> <channel> (<position> | [patchsize])
       <image>	    a ppm file 
       <mode>       map for heightmap, h or v for horizontal or vertical 1D slice 
       <channel>    r[ed], g[reen], b[lue] or l[uminance]
       <position>   if 1D slice: column/row position to slice through
       [patchsize]  if heightmap: size of the sample (default: patchesize 10 = 10x10 pixel patches
       <min> <max>  if heightmap: min/max value for color palette\n\n"
 exit 
fi

file=$1
if [ ! -e $file ] ; then 
  echo "error: file not found $file"
  exit -1
fi

mkdir -p tmp
tmpfile=tmp/tmp_plot_$$.dat


#plotargs=";set zrange[0:1];  "
# height map
if [ $2 = "hmap" ] || [ $2 = "hmap2d" ] || [ $2 = "grid" ] ; then 
  case $3 in
    r*) channel=0;;
    g*) channel=1;;
    b*) channel=2;;
    l*) # convert luminance before plotting
        channel=-1;
        
      ;;
    *) echo "error: specify channel via 3rd argument (red/green/blue/luminance)" ; exit -1;;
  esac;

  patchsize=10
  if [ -n "$4" ] ; then patchsize=$4; fi

  min=0
  mid=0.5
  max=1
  if [ -n "$5" ] && [ -n "$6" ]  ; then
     min=$5
     max=$6
     mid=`echo \($min + $max\)/2 | bc -l`
  else 
     min=`./imgtools2 -i $file -s | grep min: | cut -d':' -f2`
     echo min is $min , max is $max
     max=`./imgtools2 -i $file -s | grep max: | cut -d':' -f2`
  fi
  if [ "$7" = "log" ] ; then
     logarg="  -L "
  else 
     logarg=" "
  fi
  cbrangeargs="set cbrange [$min:$max];"
#  paletteargs="set palette defined ($min \"black\", $mid \"yellow\", $max \"red\");"
#  echo $paletteargs
 
 if [ "$channel" = -1 ] ; then
  channel=0;
  ./imgtools2 -i $file -l $logarg -X  "$channel $patchsize $tmpfile";
 else
  ./imgtools2 -i $file  $logarg  -X "$channel $patchsize $tmpfile";
 fi 
  if [ $2 = "grid" ] ; then
    gnuplot -p -e "set dgrid3d 30 30;  splot '$tmpfile' w l  t \"$file\""
  elif [ $2 = "hmap" ] ; then  
    gnuplot -p -e "$plotargs; set pm3d; $cbrangeargs splot '$tmpfile' w pm3d  t \"$file\""
  elif [ $2 = "hmap2d" ] ; then  
    gnuplot -p -e "$plotargs; set yrange [] reverse ; set pm3d map; $cbrangeargs splot '$tmpfile'   t \"$file\""
  fi
   

#horizontal/vertical slice
elif [ $2 = "h" ]  || [ $2 = "v" ] ; then
  if [ -z $4 ] ; then echo "error: specify slice col/row via 4th argument"; exit -1; fi
  min=0
  max=1
  if [ -n "$5" ] && [ -n "$6" ]  ; then
     min=$5
     max=$6
  fi
  rangeargs="set yrange [$min:$max];"

case $3 in
    r*) channel=0;; #red
    g*) channel=1;; #green
    b*) channel=2;; #blue
    l*) 
     #luminance
     echo "luminance not implemented"
     exit -1
     ;;
    *)
     # three channels simultaneously
     modearg=`echo $2 | tr [[:lower:]] [[:upper:]]`
     for channel in 0 1 2 ; do
       ./imgtools2 -i $file -$modearg $4 | grep "channel $channel" -A1 | tail -n +2 | sed 's/ /\n/g' >  ${tmpfile%.dat}_$channel.dat
     done
     gnuplot -p -e "$rangeargs plot '${tmpfile%.dat}_0.dat' w l t \"$file\", '${tmpfile%.dat}_1.dat' w l t \"$file\", '${tmpfile%.dat}_2.dat' w l t \"$file\""  
     exit 0
     ;;  
 esac;
  # single channel
  modearg=`echo $2 | tr [[:lower:]] [[:upper:]]`
  ./imgtools2 -i $file -$modearg $4 | grep "channel $channel" -A1 | tail -n +2 | sed 's/ /\n/g' >  $tmpfile
  gnuplot -p -e "$rangeargs plot '$tmpfile' w l"  

else 
  echo "error: unknown plotmode \"$2\"" 
  exit -1
fi


    



