#~/bin/bash

# evaluation of border sizes; overlap on chrome surface

source ./config


# target directory
outdir="`pwd`/hdr_beleuchtung"

#experiment source dir
resdir=$idir/experiments/hdr

 width=230
 height=160

mask=$outdir/mask.png
crop=""
tmpfile=`pwd`/tmp_plot_$$.dat

cd $idir;

plotsize="600,450"
# ldr capture for all hdr sizes
for size in single size10 size20 size100 size100hdr; do

 if [ $size = "size100hdr" ] ; then
  file=$resdir/size100/result.exr
 else 
  file=$resdir/$size/2_eramp3_10_res.exr
 fi
  outfile=$outdir/hdr_luminance_logmap_$size.png
  # apply mask here ?

  patchsize=1
  min=-3
  max=0
  cbrangeargs="set cbrange [$min:$max];"
#  paletteargs="set palette defined ($min \"black\", $mid \"yellow\", $max \"red\");"
#  echo $paletteargs

  
  channel=0;
  ./imgtools2 -i $file -F 1.0 -l -L -X  "$channel $patchsize $tmpfile";
 
 echo " `map_plotargs $plotsize `;" \
        "set output '${outfile}';"\
        "set xlabel '';" \
        "set ylabel '';" \
        "set yrange [];" \
        "set yrange[0:$height]; set xrange[0:$width];"\
        "unset xtics;" \
        "unset ytics;" \
        "set cbrange [$min:$max];"\
        "set cbtics (\"1\" 0, \"0.1\" -1, \"0.01\" -2,  \"0.001 \" -3);"\
        "set cblabel 'L';"\
        "splot '$tmpfile'  u 1:2:3 t \"\"" | gnuplot

 convert $outfile -trim +repage $outfile

done

