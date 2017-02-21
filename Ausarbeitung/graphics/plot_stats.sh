#!/bin/bash

#
# plot stats of full illumination
#

source ./config


# target directory
outdir="`pwd`/ergebnisse"

#experiment source dir
resdir=$idir/experiments/cat/grace_drago_g2

width=600
height=600 
 

#echo -n > $outdir/illum_positions.dat
## get 3d position (forward vector) for each illumination 
#for i in {0..77}; do 
#
# if [ $i -eq 24 ] ; then continue; fi
#
# # echo -n "$i " >> $outdir/illum_positions.dat
#  echo  "0 0 0 " >> $outdir/illum_positions.dat
#  egrep "^$i Ant.*"  $resdir/tracking.log | tail -n1 | cut -d'(' -f5 | cut -d')' -f1 >> $outdir/illum_positions.dat
#
#done
#
## lines
#echo "set view equal xyz; set xrange[-550:550]; set yrange[-550:550]; set zrange[-300:550]; splot '$outdir/illum_positions.dat' w l" | gnuplot -p

## points
#echo "set view equal xyz; set xrange[-550:550]; set yrange[-550:550]; set zrange[-300:550]; splot '$outdir/illum_positions.dat' w p ps 3 pt 3" | gnuplot -p

## surface
#echo " set xrange[-550:550]; set yrange[-550:550]; set zrange[-300:550]; set dgrid3d 10,10 ;  ;set pm3d ; splot '$outdir/illum_positions.dat' pal " | gnuplot -p



#
# time between image captures
#

echo -n > $outdir/delays.dat
c=0
b=0;
for i in {0..76} ; do
if [ ! $i -eq 61 ] ; then   
 tsa=`stat -c "%Y" $resdir/result/$i.cr2`
 b=`echo $i+1|bc`
 tsb=`stat -c "%Y" $resdir/result/${b}_df.cr2`
 echo $i `echo \($tsb-$tsa\)/60 |bc -l` >> $outdir/delays.dat
 c=`echo $c + \($tsb - $tsa\) | bc`
fi
done
echo total $c
echo " plot '$outdir/delays.dat' with boxes" | gnuplot -p

