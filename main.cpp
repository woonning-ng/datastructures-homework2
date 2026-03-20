#include "math.hpp"
#include "Helper.hpp"
#include "Polygon.hpp"


#include <iostream>
//Ring id --> key
// Vector<int> --> vector of vertex ids for that ring --> value
std::map <int, std::vector<Vertex>> polygon;


// MAIN FUNCTION
int main(int argc, char *argv[]) {
    Read_CSV(argv[1], polygon);
    Print_Poly(argv[2], polygon);
    return 0;
}