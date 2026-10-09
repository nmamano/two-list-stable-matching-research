CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17
PYTHON ?= python3

.PHONY: all test test-full-cover clean
all: build/reference build/best build/rbgs

build:
	mkdir -p $@

build/reference: src/reference/main.cpp $(wildcard src/reference/*.hpp) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/best: src/portfolio/best.cpp $(wildcard src/portfolio/*) $(wildcard src/rbgs/*) | build
	$(CXX) $(CXXFLAGS) $< -o $@

build/rbgs: src/rbgs/rbgs.cpp src/rbgs/core.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@

test: all
	$(PYTHON) verify/test.py

test-full-cover:
	$(PYTHON) verify/test_cover.py

clean:
	rm -rf build
