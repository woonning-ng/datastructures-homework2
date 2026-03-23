#include "math.hpp"
#include "Helper.hpp"
#include "Polygon.hpp"


#include <iostream>
//Ring id --> key
// Vector<int> --> vector of vertex ids for that ring --> value
std::map <int, std::vector<Vertex>> polygon;
std::map <int, Vertex*> rings_list; // use this, polygons should be stored in ringlist

// usage?
// ./simplify input.csv output.csv output_ring.csv
// e.g. ./simplify test_cases/input_original_06.csv test_cases/test06a.csv test_cases/test06b.csv

// MAIN FUNCTION
int main(int argc, char *argv[]) {
    // polygon tests
    Read_CSV(argv[1], polygon);
    Print_Poly(argv[2], polygon);
    Set_RingList(polygon);
    Vertex v = Vertex(10.0f,100.0f);
    Insert_Vertex(rings_list[0], &v);
    Delete_Vertex(&v);
    Print_RingList(rings_list, argv[3]);

    // math tests

    return 0;
}