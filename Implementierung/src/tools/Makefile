CC = g++
FLAGS =  -O3

default: imgtools
imgtools: imgtools.c
	${CC} ${FLAGS} -o imgtools.o  -c imgtools.c
	${CC} ${FLAGS} -o imgtools imgtools.o 
	rm imgtools.o


LIBS =   -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d
INCLUDES = -I/usr/local/include/

imgtools2: imgtools2.cpp 
	${CC} ${FLAGS} -o imgtools2.o  -c imgtools2.cpp $(INCLUDES)
	${CC} ${FLAGS} -o imgtools2 imgtools2.o  $(LIBS)
	rm imgtools2.o


vidtools: vidtools.cpp 
	${CC} ${FLAGS} -o vidtools.o  -c vidtools.cpp $(INCLUDES)
	${CC} ${FLAGS} -o vidtools vidtools.o  $(LIBS)
	rm vidtools.o

# using opencv v3:

LIBS_V3 =   -L../../lib/opencv-3.0.0-dev/install/lib -Wl,-rpath=./lib/opencv-3.0.0-dev/install/lib -lopencv_core -lopencv_highgui -lopencv_photo -lopencv_imgproc 
INCLUDES_V3 = -I../../lib/opencv-3.0.0-dev/install/include

hdrtools: hdrtools.cpp 
	${CC} ${FLAGS} -o hdrtools.o  -c hdrtools.cpp  $(INCLUDES_V3)
	${CC} ${FLAGS} -o hdrtools hdrtools.o  $(LIBS_V3)
	rm hdrtools.o

