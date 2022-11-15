export CC := $(shell which gcc)
export CXX := $(shell which g++)
CFLAGS = -Werror -std=c11 -I./ -O3
CXXFLAGS = -Wall -Werror -std=c++20 -I./ -Ofast -g -fopenmp -march=native -mtune=native

all: bfs
bfs: main
	@$(CXX) $(CXXFLAGS) -o bfs main.o

main: main.cpp
	@$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

clean:
	@rm -f *.o bfs