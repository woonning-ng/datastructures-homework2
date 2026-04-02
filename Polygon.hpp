/// Put struct to store information for polygon
#ifndef POLYGON_HPP
#define POLYGON_HPP

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Vertex {
    Vertex(double x_pos, double y_pos) : x(x_pos), y(y_pos) {}
    Vertex() = default;

    double x{};
    double y{};
    int original_vertex_id = -1;
    bool active = true;
    std::size_t version = 0;
    int ring_id = -1;
    mutable unsigned long long last_query_epoch = 0;

    Vertex operator+(const Vertex& other) const { return {x + other.x, y + other.y}; }
    Vertex operator-(const Vertex& other) const { return {x - other.x, y - other.y}; }
    Vertex operator*(double t) const { return {x * t, y * t}; }

    Vertex* prev = nullptr;
    Vertex* next = nullptr;
};

using Vector = Vertex;

struct Line {
    Vertex* a;
    Vertex* b;
};

extern std::map<int, std::vector<Vertex>> polygon;
extern std::map<int, Vertex*> rings_list;
extern std::vector<std::unique_ptr<Vertex>> vertex_pool;

void Set_RingList(std::map<int, std::vector<Vertex>>& poly_input);
void Print_RingList(const std::map<int, Vertex*>& rings_list, std::string file_path);
Vertex* AllocateVertex(double x, double y, int original_vertex_id, int ring_id);
void Insert_Vertex(Vertex* first, Vertex* new_vert);
void Delete_Vertex(Vertex* vert);
void TouchVertex(Vertex* vert);
void ResetRingHeadToSmallestId(std::map<int, Vertex*>& ring_heads, int ring_id);

#endif
