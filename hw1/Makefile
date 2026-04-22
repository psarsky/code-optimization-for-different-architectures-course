CXX := g++
CXXFLAGS_BASE := -std=c++20 -pipe -Wall -Wextra -Wpedantic
CXXFLAGS_ARCH := -march=znver2 -mtune=znver2
CXXFLAGS := $(CXXFLAGS_BASE) $(CXXFLAGS_ARCH)
LDFLAGS := -pthread

TARGETS := normalize0 normalize1 normalize2 normalize3 normalize4 normalize5 normalize6

.PHONY: all clean

all: $(TARGETS)

normalize0: normalize0.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS_BASE) -o normalize0 normalize0.cpp

normalize1: normalize1.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize1 normalize1.cpp

normalize2: normalize2.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize2 normalize2.cpp

normalize3: normalize3.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize3 normalize3.cpp

normalize4: normalize4.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize4 normalize4.cpp

normalize5: normalize5.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize5 normalize5.cpp

normalize6: normalize6.cpp normalize_common.hpp
	$(CXX) $(CXXFLAGS) -o normalize6 normalize6.cpp $(LDFLAGS)

clean:
	rm -f $(TARGETS)
