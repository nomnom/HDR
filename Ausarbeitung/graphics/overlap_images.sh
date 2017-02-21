#~/bin/bash

# evaluation of border sizes; overlap on chrome surface

source ./config


# target directory
outdir="`pwd`/beleuchtung"

#experiment source dir
resdir=$idir/experiments/eval_overlap/

 
crop="-crop ${width}x${height}+855+845"
#crop=""

cd $idir;

 plotsize="500,400"  #"`echo $pw+$border | bc`,`echo $ph + $border |bc`"
#
# on object
#

 width=140
 height=160 
for size in 0 40 80; do

 outfile=$outdir/overlap_eval_$size.exr
 infile=$resdir/$size/result.exr
  
 #crop 
 convert $infile  $crop $outfile 
 convert $outfile ${outfile%.exr}.png  

# luminance heatmap 
 infile=${outfile}
 outfile=$outdir/overlap_eval_${size}_map.png

  patchsize=1
  min=0
  max=`./imgtools2 -i $infile -s | grep max: | cut -d':' -f2`
  tmpfile=`pwd`/tmp_plot_$$.dat
  echo "min: " $min ";  max": $max
  ./imgtools2 -i $infile -l -X "0  $patchsize $tmpfile";
  
  echo " `map_plotargs $plotsize `;" \
        "set output '${outfile}';"\
        "set xlabel '';" \
        "set ylabel '';" \
        "set yrange [] reverse;" \
        "set yrange[0:$height]; set xrange[0:$width];"\
        "unset xtics;" \
        "unset ytics;" \
        "set cbrange [0:1.0];"\
        "set cblabel 'Luminanz';"\
        "splot '$tmpfile'  u 1:2:(\$3/0.119385) t \"\"" | gnuplot

   rm $tmpfile

 convert $outfile -trim +repage $outfile
done

#
# on cubemap
#
# luminance heatmap 

 width=380
 height=388
 plotsize="400,400"  #"`echo $pw+$border | bc`,`echo $ph + $border |bc`"
for size in 0 40 80; do
 #plotsize="`echo $width+$border | bc`,`echo $height + $border |bc`"
 infile=$outdir/overlap_eval_${size}_cube.exr
 outfile=$outdir/overlap_eval_${size}_cube_map.png

  patchsize=1
  min=0
  max=`./imgtools2 -i $infile -s | grep max: | cut -d':' -f2`
  tmpfile=`pwd`/tmp_plot_$$.dat

  ./imgtools2 -i $infile -l -X "0  $patchsize $tmpfile";
  
  echo " `map_plotargs $plotsize `;" \
        "set output '${outfile}';"\
        "set xlabel '';" \
        "set ylabel '';" \
        "set yrange [] reverse;" \
        "set yrange[0:$height]; set xrange[0:$width];"\
        "unset xtics;" \
        "unset ytics;" \
        "set cbrange [0:1];"\
        "set cblabel 'Luminanz';"\
        "splot '$tmpfile'  u 1:2:(\$3/$max/2) t \"\"" | gnuplot

   rm $tmpfile

 convert $outfile -trim +repage $outfile
done



# old difference experiment code : 

# declare -a sizes=('0' '20' '40' '80')
#crop images; create png copy for print
#for i in 0  1 2 3  ; do
# size=${sizes[i]} 
#
# outfile=$outdir/overlap_eval_$size.exr
# infile=$resdir/$size/result.exr
#  
# convert $infile  $crop $outfile 
# convert $outfile ${outfile%.exr}.png  #apply sRGB conversion here
#
#done
# 
##differences between images
#for i in 0  1 2   ; do
# size=${sizes[i]} 
#
# outfile=$outdir/overlap_eval_$size.exr
# infile=$resdir/$size/result.exr
# succ=`echo $i + 1 | bc`    
#   ./it -i $outfile -i $outdir/overlap_eval_${sizes[succ]}.exr -d a -o $outdir/overlap_eval_${size}_${sizes[succ]}_diff.exr
#
#done
# 
##differences to first image
#to=0
#for i in  1 2 3  ; do
# size_to=${sizes[to]} 
# size=${sizes[i]} 
#
# outfile=$outdir/overlap_eval_$size.exr
# infile=$resdir/$size/result.exr
# ./it -i $outfile -i $outdir/overlap_eval_$size_to.exr -d a -o $outdir/overlap_eval_${size_to}_${size}_diff.exr
#done
# 
 
 
 
