# build everything

#PROJS=calibrate_camera control_display undistort_display evaluate_display show_on_display artoolkit_test
#BINS=$(patsubst %,src/%/bin/Release/%,$(PROJS))

all: projects  imgtools imgtools2 vidtools


imgtools: src/tools/imgtools
src/tools/imgtools: src/tools/imgtools.c
	cd src/tools && make imgtools

imgtools2: src/tools/imgtools2
src/tools/imgtools2: src/tools/imgtools2.cpp
	cd src/tools && make imgtools2

vidtools: src/tools/vidtools
src/tools/vidtools: src/tools/vidtools.cpp
	cd src/tools && make vidtools


#src/%/bin/Release/%: src/%/%.cpp
#	cd src/% && make clean && make
projects:
	sh make.sh

checksvn:
	cd .. && svn status | grep -v experiments/ | grep -ve ".*\.layout" | grep -ve ".*bin/Release/.*" | grep -ve ".*bin/Debug/.*" | grep -v src/tools/imgtools | grep -v src/tools/imgtools2 | grep -ve  Literatur/.*_html/ | grep -v Literatur/stubs | grep -v Implementierung/tmp | grep -ve Literatur/.*.html | grep -v svg-converted-to.pdf

clean:
	rm tmp/tmp_*

PHONY: all
