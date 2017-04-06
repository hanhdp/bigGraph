SHELL := /bin/bash

CXX := g++ 
CXXFLAGS := -std=c++11 -g -O3 -Wall -Wno-unused-result -fcilkplus -lcilkrts -msse4.2 #-fopenmp # -DNDEBUG
#SOURCES := $(shell find . | grep -e ".cpp" | grep -v "main.cpp" | grep -v "convert" | grep -v "weighted" | grep -v "bench")
#OBJECTS := $(SOURCES:%.cpp=%.o)

all: main

main: biggraph.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@
#	g++ -std=c++11 -g -O3 -Wall -Wno-unused-result -fcilkplus -lcilkrts -msse4  -c best/biggraph.cpp -o biggraph.o
#	g++ -std=c++11 -g -O3 -Wall -Wno-unused-result -fcilkplus -lcilkrts -msse4  biggraph.o best/main.cpp -o main
#	gcc -fcilkplus -lcilkrts -msse4 -O3 -Wall -o akgroup main.c

clean:
	rm -rf main *.o *~ *.*~
usig: main
	./harness corpus/init-file.txt corpus/workload-file415_1M.txt corpus/result-file415_1M.txt ./main
ulive: main
	./harness corpus/init-livejournal.txt corpus/workload-livejournal415_1M.txt corpus/result-livejournal415_1M.txt ./main
upokec: main
	./harness corpus/init-pokec.txt corpus/workload-pokec415_1M.txt corpus/result-pokec415_1M.txt ./main

sig: main
	./harness corpus/init-file.txt corpus/workload-file.txt corpus/result-file.txt ./main
live: main
	./harness corpus/init-livejournal.txt corpus/workload-livejournal.txt corpus/result-livejournal.txt ./main
pokec: main
	./harness corpus/init-pokec.txt corpus/workload-pokec.txt corpus/result-pokec.txt ./main
bsig: main
	./harness corpus/init-file.txt corpus/workload-file.txt corpus/result-file.txt ./best/main	
blive: main
	./harness corpus/init-livejournal.txt corpus/workload-livejournal.txt corpus/result-livejournal.txt ./best/main
bpokec: main
	./harness corpus/init-pokec.txt corpus/workload-pokec.txt corpus/result-pokec.txt ./best/main

hsig: main
	./harness corpus/init-file.txt corpus/workload-file.txt corpus/result-file.txt ./H_minor_free/main
hlive: main
	./harness corpus/init-livejournal.txt corpus/workload-livejournal.txt corpus/result-livejournal.txt ./H_minor_free/main
hpokec: main
	./harness corpus/init-pokec.txt corpus/workload-pokec.txt corpus/result-pokec.txt ./H_minor_free/main
