
# convert debevec hdr format
pfsin $1 | pfspanoramic polar+polar -w 1366 -h 768 -o 4 | pfsout ${1%.exr}_wxga.exr
