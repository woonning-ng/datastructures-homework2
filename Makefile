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
	./$(TARGET) test_cases/input_blob_with_two_holes.csv 17 > ./output/output_blob_with_two_holes.txt
	diff test_cases/output_blob_with_two_holes.txt ./output/output_blob_with_two_holes.txt
	./$(TARGET) test_cases/input_cushion_with_hexagonal_holes.csv 13 > ./output/output_cushion_with_hexagonal_holes.txt
	diff test_cases/output_cushion_with_hexagonal_holes.txt ./output/output_cushion_with_hexagonal_holes.txt
	./$(TARGET) test_cases/input_lake_with_two_islands.csv 17 > ./output/output_lake_with_two_islands.txt
	diff test_cases/output_lake_with_two_islands.txt ./output/output_lake_with_two_islands.txt
	./$(TARGET) test_cases/input_original_01.csv 99 > ./output/output_original_01.txt
	diff test_cases/output_original_01.txt ./output/output_original_01.txt
	./$(TARGET) test_cases/input_original_02.csv 99 > ./output/output_original_02.txt
	diff test_cases/output_original_02.txt ./output/output_original_02.txt
	./$(TARGET) test_cases/input__original_03.csv 99 > ./output/output_original_03.txt
	diff test_cases/output_original_03.txt ./output/output_original_03.txt
	./$(TARGET) test_cases/input__original_04.csv 99 > ./output/output_original_04.txt
	diff test_cases/output_original_04.txt ./output/output_original_04.txt
	./$(TARGET) test_cases/input__original_05.csv 99 > ./output/output_original_05.txt
	diff test_cases/output_original_05.txt ./output/output_original_05.txt
	./$(TARGET) test_cases/input__original_06.csv 99 > ./output/output_original_06.txt
	diff test_cases/output_original_06.txt ./output/output_original_06.txt
	./$(TARGET) test_cases/input__original_07.csv 99 > ./output/output_original_07.txt
	diff test_cases/output_original_07.txt ./output/output_original_07.txt
	./$(TARGET) test_cases/input__original_08.csv 99 > ./output/output_original_08.txt
	diff test_cases/output_original_08.txt ./output/output_original_08.txt
	./$(TARGET) test_cases/input__original_09.csv 99 > ./output/output_original_09.txt
	diff test_cases/output_original_09.txt ./output/output_original_09.txt
	./$(TARGET) test_cases/input__original_10.csv 99 > ./output/output_original_10.txt
	diff test_cases/output_original_10.txt ./output/output_original_10.txt
	./$(TARGET) test_cases/input_rectangle_with_two_holes_scaled.csv 11 > ./output/output_rectangle_with_two_holes_scaled.txt
	diff test_cases/output_rectangle_with_two_holes_scaled.txt ./output/output_rectangle_with_two_holes_scaled.txt
	./$(TARGET) test_cases/input_wavy_with_three_holes.csv 21 > ./output/output_wavy_with_three_holes.txt
	diff test_cases/output_wavy_with_three_holes.txt ./output/output_wavy_with_three_holes.txt

clean:
	rm -f $(TARGET) $(OBJ)