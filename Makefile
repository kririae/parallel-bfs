export CC := $(shell which gcc)
export CXX := $(shell which g++)
CFLAGS = -Werror -std=c11 -I./ -O3
CXXFLAGS = -Werror -std=c++20 -I./ -O3 -g -fopenmp -ltbb -ltbbmalloc

all: bfs
bfs: main.cpp bfs.hpp graph.hpp
	@$(CXX) $(CXXFLAGS) -o bfs main.cpp

clean:
	@rm -f *.o bfs