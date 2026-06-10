CXX := g++
CXXFLAGS_BASE := -O2 -std=c++20 -pipe -Wall -Wextra
CXXFLAGS_ARCH := -march=znver2 -mtune=znver2
CXXFLAGS := $(CXXFLAGS_BASE) $(CXXFLAGS_ARCH)

TARGETS := gauss0 gauss1 gauss2

.PHONY: all clean

all: $(TARGETS)

gauss0: gauss0.cpp gauss_common.hpp
	$(CXX) $(CXXFLAGS_BASE) -o gauss0 gauss0.cpp

gauss1: gauss1.cpp gauss_common.hpp
	$(CXX) $(CXXFLAGS) -o gauss1 gauss1.cpp

gauss2: gauss2.cpp gauss_common.hpp
	$(CXX) $(CXXFLAGS) -o gauss2 gauss2.cpp

clean:
	rm -f $(TARGETS)
