#!/bin/bash

#
# gather statistics and plots ofr illumination run
#

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID"; exit 1; fi

explog=$3
if [ -z $explog ] ; then explog=exposures.log; fi

odir=experiments/$id/$subid
log=$odir/lightstage.log
tlog=$odir/tracking.log 

echo " Statistics for illumination run $id/$subid:"

max=`tail -n1 $odir/$explog | cut -d' ' -f1`
numills=`echo $numills + 1 | bc`
echo number of illuminations: $numills
numruns=`grep -c "START" $log`
echo "   number of starts/stops:" $numruns
#numl=`grep -c "CAPTURE BEGIN" $log`
#echo "   number of illuminations:" $numl
numfailed=`grep -c "ERROR" $log`
failed=`grep "ERROR" $log | cut -d' ' -f1 | tr '\n' ' ' ` 
echo "    .. failed: $numfailed ( $failed )"
numsucc=`echo $numills/2-$numfailed | bc `
echo "   .. successfull:" $numsucc

echo "capture begin: `ls -la experiments/cat/grace_drago_g2/result/0_df.cr2 | cut -d' ' -f9`"
echo "capture end: " `ls -la experiments/cat/grace_drago_g2/result/${max}_df.cr2 | cut -d' ' -f9`

# average calc times
echo "clearing frames: "`grep zeroing\ took $log | cut -d' ' -f 5 | awk "{ s=s+\\$1;i++} END {print s/i} "`
echo "fw projection: " 	`grep forward\ projection $log | cut -d' ' -f 7 | awk "{ s=s+\\$1;i++} END {print s/i} "`
echo "bw projection: " 	`grep backward\ projection $log | cut -d' ' -f 7 | awk "{ s=s+\\$1;i++} END {print s/i} "`
dump=`grep dumping\ took $log | cut -d' ' -f 4 | awk "{ s=s+\\$1;i++} END {print s/i} "`
echo "dumping data:" $dump
total=`grep postprocessing $log | cut -d' ' -f 4 | awk "{ s=s+\\$1;i++} END {print s/i} "`
echo "total postproc" $total
postproc=`echo $total-$dump | bc -l`
echo "postproc: $postproc" 
echo "frame calc :" 	`grep frame\ calculation $log | cut -d' ' -f 5 | awk "{ s=s+\\$1;i++} END {print s/i} "`



# number of visible markers
#grep 'Frame' $tlog | cut -d ' ' -f 14 > tmp/tmp.dat
#echo "plot 'tmp/tmp.dat' w boxes t \"Marker\"" | gnuplot -p
#rm tmp/tmp.dat

# screen center position
#grep -e 'fw (' $tlog | grep -v frame | cut -d'(' -f5 | cut -d' ' -f2-4 > tmp/tmp.dat
#echo "splot 'tmp/tmp.dat' u 1:2:3 w p " | gnuplot -p
#rm tmp/tmp.dat




#
# filter out double entries (repeated positions)
#
tmplog=tmp/tmp.log
echo -n > $tmplog
for i in `seq 0 1 $max`; do
  egrep "^$i " $tlog | grep -v frame | grep -v Frame | tail -n 1 >> $tmplog
done
tlog=$tmplog




# screen center position
#cat  $tlog | grep -v frame | grep -v Frame | cut -d'(' -f1,4 | cut -d' ' -f2-4 > tmp/tmp.dat
#echo "splot 'tmp/tmp.dat' u 1:2:3 w p ps 3 lt palette" | gnuplot -p
echo "set xrange[-550:550]; set yrange [-550:550]; set zrange [-200:550]; set terminal wxt size 500,500; splot '$tlog' u 22:23:24:1 w l lt palette" | gnuplot -p

exit;

# tracking marker on first frame
cat $tlog | grep -v Frame | grep -v frame | cut -d ' ' -f 7 > tmp/tmp.dat
echo "plot 'tmp/tmp.dat' w boxes t \"Marker\"" | gnuplot -p
rm tmp/tmp.dat

# tracking error on first frame
cat $tlog | grep -v Frame | grep -v frame | cut -d ' ' -f 4 > tmp/tmp.dat
echo "plot 'tmp/tmp.dat' w boxes t \"Error\"" | gnuplot -p
rm tmp/tmp.dat

# screen angle  on first frame
cat $tlog | grep -v Frame | grep -v frame | cut -d '=' -f 4 > tmp/tmp.dat
echo "plot 'tmp/tmp.dat' w boxes t \"Angle\"" | gnuplot -p
rm tmp/tmp.dat










