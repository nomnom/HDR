
#
# show images and environment maps on display
#

NAME = artoolkit_test

CC = g++
FLAGS =  -std=c++11 -O3 
DBGFLAGS =  

SRCS = $(wildcard *.cpp)
HDRS  = $(wildcard *.h)
OBJS = $(patsubst %.cpp,obj/Release/%.o,$(SRCS))
DBGOBJS = $(patsubst %.cpp,obj/Debug/%.o,$(SRCS))

UNAME_S := $(shell uname -s)


LIBS =  -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_calib3d -L../../lib/ARToolKit/lib  -lAR -lARMulti 
INCLUDES = -I../../lib/ARToolKit/include/ -I/usr/local/include/

all: Release


Release: bin/Release/$(NAME)

bin/Release/$(NAME): $(OBJS)
	${CC} ${FLAGS} -o $@ $<  $(LIBS)

Debug:  bin/Debug/$(NAME)

bin/Debug/$(NAME): $(DBGOBJS)
	${CC} ${DBGFLAGS} -o $@ $<  $(LIBS)

obj/Release/%.o: %.cpp %.h
	${CC} ${FLAGS} -o $@ -c $< $(INCLUDES)

obj/Debug/%.o: %.cpp %.h
	${CC} ${DBGFLAGS} -o $@ -c $< $(INCLUDES)

cleanRelease:
	rm $(OBJS)
cleanDebug:
	rm $(DBGOBJS)

cleanall: cleanRelease cleanDebug
	rm bin/*/$(NAME)
