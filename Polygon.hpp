///Put struct to store information for polygon
#include <map>
#include <utility>
#include <string>
#include <vector>
#ifndef POLYGON_HPP
#define POLYGON_HPP

struct Vertex{
    Vertex(double x_pos, double y_pos) : x(x_pos), y(y_pos){};
    Vertex(){
        x = 0.0f;
        y = 0.0f;
        active = true;
        version = 0;
        ring_id = -1;
        prev = nullptr;
        next = nullptr;
    };
    double x{};          // x coord
    double y{};          // y coord
    bool active = true; // since lines can be collapsed therefore removing
                        // vertices and we are using pointers to Vertices in
                        // lines, to avoid dangling pointers we can just set
                        // vertices to false
    std::size_t version = 0; // increment whenever neighborhood/position changes
    int ring_id = -1;

    // operator overloads
    Vertex operator+(const Vertex& other) const { return { x + other.x, y + other.y }; }
    Vertex operator-(const Vertex& other) const { return { x - other.x, y - other.y }; }
    Vertex operator*(double t) const { return { x * t, y * t }; }
    
    //Vertex --> Linked List
    Vertex * prev = nullptr;
    Vertex * next = nullptr;
};

// since Vector and Vertex are represented the same way
typedef Vertex Vector;

struct Line{
    Vertex * a;
    Vertex * b;
};

//Ring id --> key
// Vector<int> --> vector of vertex ids for that ring --> value
extern std::map <int, std::vector<Vertex>> polygon;
extern std::map <int, Vertex*> rings_list;

void Set_RingList(std::map <int, std::vector<Vertex>> & poly_input);
void Print_RingList(std::map <int, Vertex*> const& rings_list, std::string file_path);
void Insert_Vertex(Vertex * first, Vertex * new_vert);
void Delete_Vertex(Vertex * vert);
void TouchVertex(Vertex* vert);
#endif
