CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET := simplify
SRC := main.cpp Helper.cpp math.cpp Polygon.cpp Simplifier.cpp
OBJ := $(SRC:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) test_cases/input_rectangle_with_two_holes_scaled.csv 11

clean:
	rm -f $(TARGET) $(OBJ)
