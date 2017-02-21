#!/bin/bash

source ./config

#
# create heatmap plots of angled  linear ramps
#
graphdir="`pwd`/beleuchtung"

tmpfile=`pwd`/tmp_plot_$$.dat

cd $idir;


ids="0  0,v-5  0,v+5  h-5  h+5  h-5,v-5  h-5,v+5  h+5,v-5  h+5,v+5"
for id in $ids; do 

  plotsize="650,400"
#1) luminance heatmap of linear ramp 
  dir=$idir/experiments/angular/$id
  graphfile=$graphdir/verify_angular_${id}_ramps_map.png
  file=$dir/4_ramps_undist.exr

  # lowpass filter
#  tmpfile2=${tmpfile%.dat}.exr
#  ./imgtools2 -i $file -g 7 -o $tmpfile2  
#  file=$tmpfile2
 
  patchsize=1
  min=0
  max=`./imgtools2 -i $file -s | grep max: | cut -d':' -f2`
if [ -z $1 ] ; then

  ./imgtools2 -i $file -l -X "0  $patchsize $tmpfile";

  echo " `map_plotargs $plotsize `;" \
        "set output '${graphfile}';"\
        "set xlabel 'x';" \
        "set ylabel 'y';" \
        "set yrange [] reverse;" \
        "set yrange[0:767]; set xrange[0:1365];"\
        "set cbrange [0:1];"\
        "set cblabel 'L';"\
        "splot '$tmpfile'  u 1:2:(\$3/$max) t \"Luminanz\"" | gnuplot 
   
        #"set xrange[0:`echo 1366/$patchsize | bc -l`];"\
        #"set yrange [0:`echo 768/$patchsize | bc -l`] reverse;" \
 rm $tmpfile
  
 convert $graphfile -trim +repage $graphfile

fi

#
#horizontal/vertical slice
#

  plotsize=500,140
#
# r/g/b in one graph

for pos in 99 384 667 ; do
   graphfile=$graphdir/verify_angular_${id}_ramps_plot_$pos.svg
     for channel in 0 1 2 ; do
       ./imgtools2 -i $file -H $pos | grep "channel $channel" -A1 | tail -n +2 | sed 's/ /\n/g' >  ${tmpfile%.dat}_$channel.dat
     done

 echo " `graph_plotargs $plotsize `;" \
        "$graph_ls_rgb;"\
        "set output '${graphfile}';"\
        "set xlabel 'x';" \
        "set ylabel 'L';" \
        "set yrange [0:1.3];" \
        "plot '${tmpfile%.dat}_0.dat' u (\$1/0.1627)  w l t \"Zeile `echo $pos + 1 | bc`\" ls 1,"\
        "     '${tmpfile%.dat}_2.dat' u (\$1/0.1627)  w l t \"\" ls 3,"\
        "     '${tmpfile%.dat}_1.dat' u (\$1/0.1627)  w l t \"\" ls 2" | gnuplot
   

  rm ${tmpfile%.dat}_{0,1,2}.dat

#
# luminance
#    
 
   graphfile=$graphdir/verify_angular_${id}_ramps_plot_lum_$pos.svg
   ./imgtools2 -i $file -l -H $pos | grep "channel 0" -A1 | tail -n +2 | sed 's/ /\n/g' >  $tmpfile
 echo " max = " `sort -g $tmpfile | tail -n1`

 echo " `graph_plotargs $plotsize `;" \
        "$graph_ls;"\
        "set output '${graphfile}';"\
        "set xlabel 'x';" \
        "set ylabel 'L';" \
        "set yrange [0:1.1];"\
        "plot '$tmpfile' u (\$1/0.1627) w l t 'Zeile `echo $pos + 1 | bc`'  ls 1" | gnuplot 

 rm $tmpfile

 done

done
