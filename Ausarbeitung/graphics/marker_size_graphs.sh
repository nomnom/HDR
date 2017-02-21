#~/bin/bash

# positionserkennung: marker size evaluation

source ./config


# plot phi and theta of cam position for three rotating cubes with different size 

# immage sources
dir=experiments/marker/1

# graph target directory
graphdir="`pwd`/position"

thresh=50


# everything runs in the impl. directory
cd $idir;

declare -a vids=('rot3' 'rot1' 'rot2')
#declare -a vids=('rotfast1' 'rotfast2' 'rotfast3')
declare -a sizes=('1x1' '2x2' '3x3')
declare -a nummarker=('2' '8' '18')
declare -a numframes=('150' '195' '195')

plotsize="600,150"

for i in 0 1 2 ; do

  logfile=$dir/${vids[i]}.log
  if [ ! -e $logfile ] || [ "$1" = "clean" ] ; then   
     # create tracking data
    ./artoolkit_test data/camera/lenovo_parameters.yml --multi data/marker/testcube_${sizes[i]}.dat $thresh false false $dir/${vids[i]}_%01d.bmp $logfile 
  fi
 
  # clean logfile
  cat $logfile |  tr -d "[]" > tmp.log
  mv tmp.log $logfile
  
  graphfile=$graphdir/graph_marker_eval_${sizes[i]}
  mmin=`echo ${nummarker[i]}/2 | bc`
  mmax=${nummarker[i]}
  
  #plot graphs
  echo " `graph_plotargs $plotsize `;" \
        "set output '$graphfile.svg';"\
        "$graph_ls;" \
        "set key horizontal;"\
        "set xlabel '';" \
        "set xtics (\"0°\" 0, \"15°\" 15, \"30°\" 30, \"45°\" 45, \"60°\" 60, \"75°\"  75,  \"90°\" 90 ) ; " \
        "set ylabel 'Abstand (mm)';" \
        "set ytics 495,5,510 nomirror;"\
        "set yrange[480:520];" \
        "set y2label 'Fiducials';" \
        "set y2tics 1;"\
        "set y2tics ('$mmin' $mmin, '$mmax' $mmax) ;" \
        "set y2range [`echo $mmin - $mmax/6.0 | bc -l`:`echo $mmax \* 2.5 | bc `];" \
        "plot '$logfile' u (\$1/${numframes[i]}*90):4  w l t 'Abstand (mm)' ls 1 axis x1y1 ," \
        "     '$logfile' u (\$1/${numframes[i]}*90):3 w l t 'Fiducials' ls 3 axis x1y2 " | gnuplot 
 #
#        
##"set y2range[0:`echo ${nummarker[i]} \* 4 | bc` ];" \
# #distance and number of marker as two separate graphs
#  echo " `graph_plotargs $plotsize `;" \
#        "set output '${graphfile}_distance.svg';"\
#        "$graph_ls;" \
#        "set xlabel 'Sekunden';" \
#        "set xrange[0:];"\
#        "set ylabel 'Abstand (mm)';" \
#        "set yrange[495:515];" \
#        "plot '$logfile' u (\$1/${numframes[i]}*8):4 w l t '' ls 1 " | gnuplot 
# 
#  echo " `graph_plotargs $plotsize `;" \
#        "set output '${graphfile}_marker.svg';"\
#        "$graph_ls;" \
#        "set xlabel 'Sekunden';" \
#        "set xrange[0:];"\
#        "set ylabel 'Anzahl Marker';" \
#        "set yrange[0:];" \
#        "set offset 0,0,`echo ${nummarker[i]} \* 0.25 | bc -l`,0;"\
#        "plot '$logfile' u (\$1/${numframes[i]}*8):3 w l t '' ls 1 " | gnuplot 
# 
# 
 
 done
  
 
 
 
 
 
 
 
 
 
