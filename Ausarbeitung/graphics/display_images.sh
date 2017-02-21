#~/bin/bash

# heatmap plots of display backlight (100%/50%/10% uniform grey)

source ./config

#
#
#  close : 50cm distance
#  
#
# target directory
outdir="`pwd`/beleuchtung"

 width=800
 height=450
 border=200
 plotsize="`echo $width+$border | bc`,`echo $height + $border |bc`" 

tmpfile=`pwd`/tmp_plot_$$.dat
tmpimg=`pwd`/tmp_img_$$.exr

cd $idir;


declare -a ids=('svr32_close_single' 'svr32')
declare -a names=('close' 'distant')
declare -a greys=('04' '16' '32')
                                #ss       #fl   #ap                                #ss           #fl    #ap
#declare -a factors=("`echo 1.0 / 2.5 / \(24.0\*22.0\)^2 | bc -l`" "`echo 1.0 / \(1.0/8.0\)  / \(62.0\*5.0\)^2 | bc -l`")

# scale images typewise so thhe  grey32 plot is 1.0 max
declare -a factors=("`echo 1.0/0.313232|bc -l`" "`echo 1.0/0.702637|bc -l`");
 
for n in 0 1 ; do
  #experiment source dir
  resdir=$idir/experiments/lenovo/${ids[n]}


 for i in 0 1 2; do 

  infile=$resdir/3_grey_${greys[i]}_undist.exr
  outfile=$outdir/backlight_${names[n]}_${greys[i]}_map.png
  
  w=`./imgtools2 -i $infile -s | grep size | cut -d' ' -f3`
  h=`./imgtools2 -i $infile -s | grep size | cut -d' ' -f5`

  # crop a very small border and apply blur for max/min calculation 
  # (mouse cursor in the bottom right corner messes up the range scaling)
  cropsize=5
  convert $infile -crop `echo $w - $cropsize \* 2 | bc`x`echo $h - $cropsize \* 2 | bc`+$cropsize+$cropsize -blur 7x7 $tmpimg;

  min=`./imgtools2 -i $tmpimg -m ${factors[n]} -s | grep min: | cut -d':' -f2`
  max=`./imgtools2 -i $tmpimg -m ${factors[n]} -s | grep max: | cut -d':' -f2`
  echo ${names[n]} ${greys[i]} $min $max
#  paletteargs="set palette defined ($min \"black\", $mid \"yellow\", $max \"red\");"


  patchsize=2
  channel=0;
  ./imgtools2 -i $infile -l -m ${factors[n]} -X  "$channel $patchsize $tmpfile";
  echo " `map_plotargs $plotsize `;" \
        "set output '${outfile}';"\
        "set xlabel '';" \
        "set ylabel '';" \
        "set yrange [] reverse;" \
        "set yrange[0:$h]; set xrange[0:$w];"\
        "unset xtics;" \
        "unset ytics;" \
        "set cbrange [$min:$max];"\
        "set cblabel 'Luminanz';"\
        "splot '$tmpfile'  u (\$1*$patchsize):(\$2*$patchsize):3 t \"\"" | gnuplot
   convert $outfile -trim $outfile

 done
done

rm $tmpfile
rm $tmpimg
