#include "math.hpp"
#include "Helper.hpp"
#include "Polygon.hpp"


#include <iostream>
//Ring id --> key
// Vector<int> --> vector of vertex ids for that ring --> value
std::map <int, std::vector<Vertex>> polygon;
std::map <int, Vertex*> rings_list;

// usage:
// ./simplify input.csv output.csv

// MAIN FUNCTION
int main(int argc, char *argv[]) {
    // polygon tests
    Read_CSV(argv[1], polygon);
    Print_Poly(argv[2], polygon);
    Set_RingList(polygon);
    Print_RingList(rings_list, argv[3]);

    // math tests

    return 0;
}