

id=$1
if [ -z "$id" ]; then echo "Please specify the experiment ID of this calibration run using the first command line argument."; exit 1; fi

subid=$2
if [ -z "$subid" ]; then echo "Please specify the secondary ID (e.g. viewing angle)"; exit 1; fi

conf=$3
if [ -z "$conf" ]; then echo "Please specify the display config file (e.g. data/display/e330/parameters.yml) with the third argument"; exit 1; fi

size=$4
if [ -z "$size" ]; then echo "Please specify the sequence size with the fourth  argument; will perform postprocess only"; nocapture=1; fi


odir=experiments/$id/$subid

idir=`pwd`

ap=7.1
exposures="10 20 30"


if [ ! "$nocapture" = "1" ] ; then

if [ -e "experiments/$id/$subid" ] ; then
 read -n 1 -p "The ID $id/$subid already exists! Overwrite [y/n]?"
 if [ ! "$REPLY" = "y" ] ; then exit 1; fi
# rm -r $odir
 echo
fi;
mkdir -p $odir


 for ss in $exposures  ; do 
  dur=`echo $ss - 2 | bc -l`
 
  fps=`echo $size/$dur | bc -l`

  echo "capturing expramp using $size frames at $fps FPS; exposure is $ss, aperture is $ap; hdr sequence duration is $dur" 
  

  f=$odir/2_eramp3_$ss
  df=$odir/1_darkframe_$ss
  
  # capture hdr sequence
  ./show_on_display --hdr_sequence $size $fps data/display/e330/c_eramp3.exr $conf $f.cr2 $ss $ap
  
  # capture darkframe
  ./control_display 1366 768 0 --rgb 0 0 0 & 
  sleep 1.5
  ./capture.sh $ss $ap  $df noshow
  killall -KILL control_display
  
done

fi

crop="230x160+2700+1800"
#crop="130x80+1350+900"
for ss in $exposures; do

 
 echo "postprocessing... exposure $ss"

 f=$odir/2_eramp3_$ss
df=$odir/1_darkframe_$ss
  
 dcraw -4 $f.cr2 $df.cr2
  convert $f.ppm  -crop $crop $f.exr 
  convert $df.ppm -crop $crop $df.exr
 #./it -i $f.exr -i $df.exr -S -c "0 1" -r "data/camera/response_canon.m" -o ${f}_res.exr
 ./it -i $f.exr -i $df.exr -S -c "0 1" -o ${f}_res.exr

# scale so maxval is 1.0 (ok for logplot)
  maxval=`./it -i ${f}_res.exr -s | grep max: | cut -d':' -f2`
  ./it -i ${f}_res.exr -m `echo 1.0 / $maxval | bc -l` -o ${f}_res.exr
  convert ${f}_res.exr ${f}_res.ppm
 #pfsv ${f}_res.exr
  
   echo ${f}_res.ppm `echo 1 / $ss | bc -l` 1 1 0 >> $odir/exps.hdrgen
 

done

exit;
pfsinhdrgen $odir/exps.hdrgen | pfshdrcalibrate -x --bpp 16 -f data/camera/response_canon.m | pfsoutexr $odir/result.exr
#scale again
 maxval=`./it -i ${f}_res.exr -s | grep max: | cut -d':' -f2`
  ./it -i $odir/result.exr -m `echo 1.0 / $maxval | bc -l` -o $odir/result.exr
  
