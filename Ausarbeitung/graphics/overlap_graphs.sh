#~/bin/bash

# vollstaendige beleuchtung: raender ausblenden , graphen von sprung/hut/sinus funktion 

source ./config


# graph target directory
graphdir="`pwd`/beleuchtung"

graphsize="400,200"

declare -a funcs=('(x<0.5 ? 1 : 0)' '(x<0 ? 1 : ( x<1 ? 1-x : 0 ))' '(x<0 ? 1 : (x>1 ? 0 : (1-(sin(x*pi-pi/2)+1)/2)))' '' )

#  '(x<0 ? 1 : (x>1 ? 0 : (1-(sin(x*pi-pi/2)+1)/2)**0.5))')

declare -a names=('sprung' 'hut' 'sinus')

for i in 0 1 2  ; do
  for s in 0 0.25 -0.25; do
    graphfile=$graphdir/overlap_${names[i]}_$s.svg
 
  echo " `graph_plotargs $graphsize` 
        set output '${graphfile}'
        $graph_ls
        set samples 1000
        set xlabel 'Position'
        set xrange[-1:1.5]
        set ylabel 'Relative Strahldichte'
        set yrange[-0.1:2.5]
        f(x)=${funcs[i]}
        s=$s
        set style fill transparent pattern 4 bo
        set style function filledcurve y1=0
        plot f(x) ls 1 t 'Rampe A' , 1-f(x-s) ls 1 t 'Rampe B', f(x)+1-f(x-s)+0.03 w l ls 1 t 'A+B' " | gnuplot 
 
  done
done
 
 
 
 
 
 
 
 
