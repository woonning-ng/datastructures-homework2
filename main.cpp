#include "math.hpp"
#include "Helper.hpp"
#include "Polygon.hpp"


#include <iostream>

std::map<std::pair<std::string, std::string>, Vertex> polygon;

// MAIN FUNCTION
int main(int argc, char *argv[]) {
    std::cout << argv[1];
   return Read_CSV(argv[1], polygon);
}