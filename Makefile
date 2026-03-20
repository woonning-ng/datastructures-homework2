# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++23 -Wall -g

# Target executable
TARGET = simplify

# Source files
SRC = main.cpp Helper.cpp math.cpp

# Default rule
all: $(TARGET)

# Build the executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# ./simplify input_file output_file_for_checking
# make run --> command to run this
run:
	./$(TARGET) test.csv output.csv

# Run with valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Clean rule
clean:
	rm -f $(TARGET)