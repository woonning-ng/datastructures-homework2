CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
SHELL := bash
PWSH ?= powershell.exe -NoProfile -ExecutionPolicy Bypass
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
	$(PWSH) -File scripts/GenerateOutputs.ps1 -NoBuild -TimeoutSeconds $(TIMEOUT_SECONDS)

compare:
	$(PWSH) -File scripts/CompareOutputs.ps1 -DiffLines $(DIFF_LINES)

test: $(TARGET)
	$(PWSH) -File scripts/RunTests.ps1 -NoBuild -TimeoutSeconds $(TIMEOUT_SECONDS) -DiffLines $(DIFF_LINES)

run: test

clean:
	rm -f $(TARGET) $(OBJ)
