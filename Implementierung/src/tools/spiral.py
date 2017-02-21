#!/usr/bin/python
import sys
import math
#for pi in range(0,snum):
#   for ti in range(0,snum):
#      p=(float)pi/float(snum-1)*math.pi
#      t=(float)t/float(snum-1)*2.0*math.pi

def x(inclin, azi):
  return math.sin(inclin) * math.cos(azi)

def y(inclin, azi):
  return math.sin(inclin) * math.sin(azi)

def z(inclin):
  return math.cos(inclin)

def h(k,N):
  return -1.0 + 2.0*(k-1.0)/(N-1.0)
   
N=700
if (len(sys.argv) > 1):
   N=int(sys.argv[1])

pk=0;
for k in range(1,N+1):
  tk=math.acos(h(k,N)) 
  if (k>1  and k<N): 
    pk = pk + (3.6)/math.sqrt(N)/math.sqrt(1.0-h(k,N)*h(k,N)) % math.pi*2
  else:
    pk=0
  print x(tk,pk), y(tk,pk), z(tk)


