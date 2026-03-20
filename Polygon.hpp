///Put struct to store information for polygon
#include <map>
#include <utility>
#include <string>
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

//Key --> ring_id, vertex_id
//Value --> Vertex
extern std::map<std::pair<std::string, std::string>, Vertex> polygon;
#endif