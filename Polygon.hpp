///Put struct to store information for polygon
#include <map>
#include <utility>
#include <string>
#include <vector>
#ifndef POLYGON_HPP
#define POLYGON_HPP
struct Vertex{
    float x{};
    float y{};
};

struct Line{
    Vertex *  a;
    Vertex * b;
};

//Ring id --> key
// Vector<int> --> vector of vertex ids for that ring --> value
extern std::map <int, std::vector<Vertex>> polygon;



#endif