CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
SHELL := bash
TARGET := simplify
SRC := main.cpp Helper.cpp math.cpp Polygon.cpp Simplifier.cpp SpatialGrid.cpp
OBJ := $(SRC:.cpp=.o)
TIMEOUT_SECONDS ?= 30
DIFF_LINES ?= 40

.PHONY: all clean run generate compare test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

generate: $(TARGET)
	NO_BUILD=1 bash scripts/GenerateOutputs.sh $(TIMEOUT_SECONDS)

compare:
	bash scripts/CompareOutputs.sh $(DIFF_LINES)

test: $(TARGET)
	NO_BUILD=1 bash scripts/RunTests.sh $(TIMEOUT_SECONDS) $(DIFF_LINES)

run: test

clean:
	rm -f $(TARGET) $(OBJ)
