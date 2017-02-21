#~/bin/bash

# positionserkennung: stage evaluation

source ./config

# graph target directory
graphdir="`pwd`/position"

plotsize="600,150"
thresh=130

# plot graphs for handheld (A,B,C,C2) and stationary (B,C,C2)

for id in handheld stationary ; do
 
# immage sources
dir=experiments/tracking/$id


# everything runs in the impl. directory
cd $idir;

avgfile=tmp_avg.log
echo -n > $avgfile
for bglight in 0 100; do

 for vid in A B C C2 ; do

  if [ $id = "stationary" ] && [ $vid = "A" ] ; then continue; fi

  logfile=$dir/${id}_${vid}_${bglight}.log

  # create tracking data
  if [ ! -e $logfile ] || [ "$1" = "clean" ] ; then   
    ./artoolkit_test data/camera/lenovo_parameters.yml --multi data/marker/stage.dat $thresh false true $dir/${vid}_${bglight}_%01d.bmp $logfile 

    # processlogfile
    cat $logfile |  tail -n +10 | head -n 32 |  tr -d "[]," > tmp.log
    cat tmp.log >> $avgfile
    mv tmp.log $logfile

  fi
    
  
  
  graphfile=$graphdir/graph_stage_eval_${id}_${vid}_${bglight}
  
  #plot graphs
 # echo " `graph_plotargs 'slim' `;" \
 #       "set output '$graphfile';"\
 #       "$graph_ls;" \
 #       "set ylabel 'Abstand (mm)';" \
 #       "set yrange[480:520];" \
 #       "set ytics 495,5,510;"\
 #       "set y2label 'Anzahl Marker';" \
 #       "set y2tics 1;" \
 #       "set y2tics ('$mmin' $mmin, '$mmax' $mmax) $mmin,$mmin,$mmax ;" \
 #       "set y2range[0:`echo ${nummarker[i]} \* 4 | bc` ];" \
 #       "plot '$logfile' u 1:4 w l t 'Abstand (mm)' ls 1 axis x1y1 ," \
 #       "     '$logfile' u 1:3 w l t 'Anzahl Marker' ls 2 axis x1y2 " | gnuplot 
 #       #"set yrange[-pi:pi];" \
 #       #"plot '$logfile' u 1:2 w l t 'fiterr'" | gnuplot 
 #       #"plot '$logfile' u 1:5 w l t 'Phi',"\
 #       #     "'$logfile' u 1:6 w l t 'Theta'" | gnuplot 
 
 #distance and number of marker as two separate graphs
  echo " `graph_plotargs $plotsize `;
        set output '${graphfile}_distance.svg';
        $graph_ls;
        set xlabel 'Sekunden';
        set autoscale
        set xrange[0:32];
        set ylabel 'Abstand (mm)';
        plot '$logfile' u 4 w l t '' ls 1 " | gnuplot 
 
  echo " `graph_plotargs $plotsize `;
        set output '${graphfile}_marker.svg';
        $graph_ls;
        set xlabel 'Sekunden';
        set xrange[0:32];
        set ylabel 'Anzahl Marker';
        set yrange[0:];
        plot '$logfile' u 3 w l t '' ls 2 " | gnuplot 
 
 
 
 done
  
 
 
 
done 
 
 
 
 
done 

 
#average error
errsum=`awk '{ a+=$2 } END {print a}' $avgfile`
echo errsum=$errsum
lines=`wc -l $avgfile | cut -d' ' -f1`
avgerr=`echo "$errsum / $lines" | bc -l`
echo "Average error is  $avgerr" 
#rm $avgfile


# in-one plot
dir=experiments/tracking
bglight=0

for vid in B C C2; do
  
  graphfile=$graphdir/graph_stage_eval_both_${vid}_${bglight}
  logh=$dir/handheld/handheld_${vid}_${bglight}.log
  logs=$dir/stationary/stationary_${vid}_${bglight}.log 
  minh=`cut -d' ' -f 4 $logh | tr -d ',' | sort | head -n1 | cut -d'.' -f1`
  maxh=`cut -d' ' -f 4 $logh | tr -d ',' | sort | tail -n1 | cut -d'.' -f1`
  mins=`cut -d' ' -f 4 $logs | tr -d ',' | sort | head -n1 | cut -d'.' -f1`
  maxs=`cut -d' ' -f 4 $logs | tr -d ',' | sort | tail -n1 | cut -d'.' -f1`
  echo $minh $mins $maxh $maxs 
  if [  "$minh" -lt "$mins" ] ; then min=$minh; else min=$mins; fi
  if [ "$maxh" -gt "$maxs" ] ; then max=$maxh; else max=$maxs; fi
  echo $max - $min | bc -l 
  ou=4
  ol=1
  #maxh=`echo $maxh + $ou | bc -l`
  maxh=`echo $minh + $ou | bc -l`
  minh=`echo $minh - $ol | bc -l`
  #maxs=`echo $maxs + $ou | bc -l`
  maxs=`echo $mins + $ou | bc -l`
  mins=`echo $mins - $ol | bc -l`
  max=`echo $max + $ou | bc -l`
  min=`echo $min - $ol | bc -l`
  echo " `graph_plotargs $plotsize `;
        set output '${graphfile}_distance.svg';
        $graph_ls;
        set key horizontal 
        set xlabel 'Sekunden';
        set xrange[0:2];
        set ylabel 'Abstand (mm)' tc ls 1
        set ytics $minh,1,$maxh tc ls 1
        set yrange [$minh:$maxh]
        set y2label 'Abstand (mm)' tc ls 2
        set y2tics $mins,1,$maxs tc ls 2
        set y2range [$mins:$maxs]
        plot '$logh' u ((\$1-10)/16):4 w l t 'Handgehalten' ls 1 axis x1y1, '$logs' u 4 w l t 'Stationaer' ls 3  axis x1y2" | gnuplot 
 
done

exit

# tracking images
 for t in handheld stationary; do 
   for i in B C C2 ; do 
    convert experiments/tracking/$t/${i}_0_10.bmp -flip -flop -negate -colorspace Gray  ../Ausarbeitung/graphics/position/trackimg_${t}_${i}.png
  done
 done

