#~/bin/bash

# plots display response curves

source ./config

size='400,300'
responsedir=experiments/lenovo/svr32_close_single

# graph target directory
graphfile="`pwd`/kalibrierung/lenovo_response"

declare -a names=('' ''  '');



# everything runs in the impl. directory
cd $idir;
if [ 0 -eq 1 ] ; then
for i in 13371337 550 551 1055; do

  ./evaluate_display --svr 23 24 30 $i tmp.dat - `for i in \`seq -w 0 1 32\`; do echo -n " $responsedir/3_grey_${i}_undist.exr "; done`
  ## as one combined plot 
  
 if [ "$i" -eq 13371337 ] ; then i=0; fi
 # normal plot 
 echo " `graph_plotargs $size `;
        set output '${graphfile}_$i.svg';
        $graph_ls_pt_rgb;
        set xlabel 'Framebuffer-Wert v_C ';
        set xrange[0:1];
        set ylabel 'relative Strahldichte l_C';
        set yrange[0:1];
        set grid;
        plot 'tmp.dat' u (\$1/32):(\$2/0.35)  w l t '${names[0]}' ls 1, 'tmp.dat' u (\$1/32):(\$3/0.35)  w l t '${names[1]}' ls 2,  'tmp.dat' u (\$1/32):(\$4/0.35)  w l t '${names[2]}' ls 3 " | gnuplot 
 
 
done
fi

## 4x3 multiplot

for x in {0..3}; do
  for y in {0..2}; do

     patchidx=`echo $y\*8\*44+$x\*11 | bc`

 #if [ $x -eq 0 ] && [ $y -eq 0 ] ; then patchidx=013371337 ;fi;
 #./evaluate_display --svr 23 24 30 $patchidx tmp_${x}_${y}.dat - `for i in \`seq -w 0 1 32\`; do echo -n " $responsedir/3_grey_${i}_undist.exr "; done`
 # ./evaluate_display --svr 23 24 30 -$patchidx tmp_inv_${x}_${y}.dat - `for i in \`seq -w 0 1 32\`; do echo -n " $responsedir/3_grey_${i}_undist.exr "; done`
  done
done


inv=""
cmd=""
for y in {0..2}; do 
 for x in {0..3}; do 
  cmd="$cmd plot 'tmp_$inv${x}_${y}.dat' u (\$1/32):(\$2/0.31)  w l lt 1 lw 2 notitle, 'tmp_$inv${x}_${y}.dat' u (\$1/32):(\$3/0.31)  w l  ls 2 lw 2 notitle,  'tmp_$inv${x}_${y}.dat' u (\$1/32):(\$4/0.31)  w l  ls 3 lw 2 notitle;";
 done
done
 

size='1000,600'

 echo " `graph_plotargs $size `;
        set output '${graphfile}_multi.svg';
        $graph_ls_pt_rgb;
        set samples 100
        set multiplot layout 3,4 rowsfirst
         set xtics 0, 0.2, 1 ; 
         set ytics 0, 0.2, 1  ; 
         set xlabel 'V'
         set ylabel 'L'
         set xrange[0:1];
         set yrange[0:1];
         $cmd        
        unset multiplot " | gnuplot 
         
 
