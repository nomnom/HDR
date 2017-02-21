#~/bin/bash

# dump a heatmap plot
#

if [ "$#" -lt 7 ] ; then 
 echo "usage:  sh heatmap.sh infile.exr outfile.png  minval maxval width height clabel xlabel ylabel"
 exit -1
fi

#heatmap setup and styles
function map_plotargs() {
  echo "set terminal png size $1 ; set key outside center top ; set pm3d map; set tics nomirror out; set xtics nomirror"
}

infile=$1; shift
outfile=$1; shift
#tics=$1; shift
min=$1; shift
max=$1; shift
width=$1; shift
height=$1; shift
clabel=$1; shift
xlabel=$1; shift
ylabel=$1; shift



# luminance heatmap 
 border=150
 plotsize="`echo $width+$border | bc`,`echo $height + $border |bc`"

  patchsize=7
#  if [ "$min" -eq "$max" ] ; then 
#    min=`./imgtools2 -i $infile -s | grep min: | cut -d':' -f2`
#    max=`./imgtools2 -i $infile -s | grep max: | cut -d':' -f2`
#  fi
  w=`./imgtools2 -i $infile -s | grep size | cut -d' ' -f3`
  h=`./imgtools2 -i $infile -s | grep size | cut -d' ' -f5`
  tmpfile=tmp_plot_$$.dat

  ./imgtools2 -i $infile -l -X "0  $patchsize $tmpfile";
  
  echo " `map_plotargs $plotsize `;" \
        "set output '${outfile}';"\
        "set xlabel '$xlabel';" \
        "set ylabel '$ylabel';" \
        "set yrange [] reverse;" \
        "set yrange[0:$h]; set xrange[0:$w];"\
        "set cbrange [$min:$max];"\
        "set cblabel '$clabel';"\
        "splot '$tmpfile'  u (\$1*$patchsize):(\$2*$patchsize):3 t \"\"" |  gnuplot

   rm $tmpfile


 
