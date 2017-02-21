#~/bin/bash

# plots canon response curve

source ./config

size='300,300'
response=data/camera/response_canon.m

# graph target directory
graphfile="`pwd`/kalibrierung/canon_response"

declare -a names=('Rotkanal' 'Grünkanal'  'Blaukanal');
declare -a c=('R' 'G'  'B');

# everything runs in the impl. directory
cd $idir;


# grep three channels into temporary file
for i in 0 1 2 ; do 
 grep "name: I${c[i]}" -A65539   $response  | tail -n+5  > tmp_$i.dat

## separate
 # normal plot 
 echo " `graph_plotargs $size `;" \
        "set output '${graphfile}_$i.svg';"\
        "$graph_ls_rgb;" \
        "set xlabel 'relative Strahldichte l_C';" \
        "set ylabel 'Sensor RAW-Wert v_C ';" \
        "set xrange[-0.1:1.1];" \
        "set yrange[-0.1:1.1];"\
        "set grid;" \
        "plot 'tmp_$i.dat' u (\$3/2.430620):(\$2/65535)  w l t '${names[i]}' lt `echo $i + 1 | bc` " | gnuplot
 
 # as log-log plot
  echo " `graph_plotargs $size `;" \
        "set output '${graphfile}_${i}_log-log.svg';"\
        "$graph_ls_rgb;" \
        "set xlabel 'relative Strahldichte: log_{10}(l_C)';" \
        "set ylabel 'Sensor RAW-Wert: log_{10}(v_C) ';" \
        "set xrange[-2.8:0.2];" \
        "set yrange[-2.8:0.2];"\
        "set grid;" \
        "plot 'tmp_$i.dat' u (log10(\$3/2.430620)):(log10(\$2/65535))  w l t '${names[i]}' lt `echo $i + 1 | bc` " | gnuplot

done

 
## as one combined plot 

 # normal plot 
 echo " `graph_plotargs $size `;" \
        "set output '${graphfile}.svg';"\
        "$graph_ls_rgb;" \
        "set xlabel 'relative Strahldichte l_C';" \
        "set ylabel 'Sensor RAW-Wert v_C ';" \
        "set xrange[-0.1:1.1];" \
        "set yrange[-0.1:1.1];"\
        "set grid;" \
        "plot 'tmp_0.dat' u (\$3/2.430620):(\$2/65535)  w l t '${names[0]}' lt 1, "\
        "     'tmp_1.dat' u (\$3/2.430620):(\$2/65535)  w l t '${names[1]}' lt 2, "\
        "     'tmp_2.dat' u (\$3/2.430620):(\$2/65535)  w l t '${names[2]}' lt 3 " | gnuplot 
 
 # as log-log plot
  echo " `graph_plotargs $size `;" \
        "set output '${graphfile}_log-log.svg';"\
        "$graph_ls_rgb;" \
        "set xlabel 'relative Strahldichte: log_{10}(l_C)';" \
        "set ylabel 'Sensor RAW-Wert: log_{10}(v_C) ';" \
        "set xrange[-2.8:0.2];" \
        "set yrange[-2.8:0.2];"\
        "set grid;" \
        "plot 'tmp_0.dat' u (log10(\$3/2.430620)):(log10(\$2/65535))  w l t '${names[0]}' lt 1, "\
        "     'tmp_1.dat' u (log10(\$3/2.430620)):(log10(\$2/65535))  w l t '${names[1]}' lt 2, "\
        "     'tmp_2.dat' u (log10(\$3/2.430620)):(log10(\$2/65535))  w l t '${names[2]}' lt 3 " | gnuplot


for i in 0 1 2 ; do 
 rm tmp_$i.dat; 
done 
 
