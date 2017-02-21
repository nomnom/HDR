
#
#  Calibrate Camera
#

NAME = calibrate_camera

CC = g++
FLAGS =  -std=c++11 -O3
DBGFLAGS = -std=c++11 -g

SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,obj/Release/%.o,$(SRCS))
DBGOBJS = $(patsubst %.cpp,obj/Debug/%.o,$(SRCS))


LIBS =   -L/usr/local/lib/  -lopencv_core -lopencv_highgui -lopencv_calib3d -lopencv_imgproc -lopencv_features2d
INCLUDES = -I/usr/local/include/

all: Release


Release: bin/Release/$(NAME)

bin/Release/$(NAME): $(OBJS)
	${CC} ${FLAGS} -o $@ $<  $(LIBS)

Debug:  bin/Debug/$(NAME)

bin/Debug/$(NAME): $(DBGOBJS)
	${CC} ${DBGFLAGS} -o $@ $<  $(LIBS)

obj/Release/%.o: %.cpp
	${CC} ${FLAGS} -o $@ -c $< $(INCLUDES)

obj/Debug/%.o: %.cpp
	${CC} ${DBGFLAGS} -o $@ -c $< $(INCLUDES)

cleanRelease:
	rm $(OBJS)
cleanDebug:
	rm $(DBGOBJS)

cleanall: cleanRelease cleanDebug
	rm bin/*/$(NAME)
