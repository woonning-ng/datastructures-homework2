#include "Polygon.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

std::vector<std::unique_ptr<Vertex>> vertex_pool;

Vertex* AllocateVertex(double x, double y, int original_vertex_id, int ring_id) {
    auto vertex = std::make_unique<Vertex>();
    vertex->x = x;
    vertex->y = y;
    vertex->original_vertex_id = original_vertex_id;
    vertex->active = true;
    vertex->version = 0;
    vertex->ring_id = ring_id;
    vertex->prev = nullptr;
    vertex->next = nullptr;

    Vertex* ptr = vertex.get();
    vertex_pool.push_back(std::move(vertex));
    return ptr;
}

//Insert a new vertex after a vertex
void Insert_Vertex(Vertex * first, Vertex * new_vert){
    Vertex * cur = first->next;
    first->next = new_vert;
    new_vert->next = cur;
    cur->prev = new_vert;
    new_vert->prev = first;
    TouchVertex(first);
    TouchVertex(cur);
    TouchVertex(new_vert);

}

//Delete a vertex
void Delete_Vertex(Vertex * vert){
    

    Vertex * next = vert->next;
    Vertex * prev = vert->prev;

    prev->next = next;
    next->prev = prev;

    vert->prev = nullptr;
    vert->next = nullptr;
    vert->active = false;
    TouchVertex(prev);
    TouchVertex(next);
    TouchVertex(vert);
}


//Set Up Doubly Linked List
void Set_RingList(std::map <int, std::vector<Vertex>> & poly_input){
    rings_list.clear();
    vertex_pool.clear();

    for (auto& [ring_id, vertices] : poly_input) {
        std::size_t sz = vertices.size();

        if (sz == 0) {
            rings_list[ring_id] = nullptr;
            continue;
        }

        std::vector<Vertex*> nodes;
        nodes.reserve(sz);
        for (const Vertex& source : vertices) {
            nodes.push_back(AllocateVertex(source.x, source.y, source.original_vertex_id, ring_id));
        }

        if (sz == 1) {
            nodes[0]->prev = nodes[0];
            nodes[0]->next = nodes[0];
            rings_list[ring_id] = nodes[0];
            continue;
        }

        for (std::size_t id = 0; id < sz; ++id) {
            nodes[id]->prev = nodes[(id + sz - 1) % sz];
            nodes[id]->next = nodes[(id + 1) % sz];
        }

        // Find vertex with smallest original_vertex_id to use as ring head
        std::size_t head_index = 0;
        for (std::size_t i = 1; i < sz; ++i) {
            if (nodes[i]->original_vertex_id < nodes[head_index]->original_vertex_id) {
                head_index = i;
            }
        }
        rings_list[ring_id] = nodes[head_index];
    }
}

void TouchVertex(Vertex* vert) {
    if (vert != nullptr) {
        ++vert->version;
    }
}

void ResetRingHeadToSmallestId(std::map<int, Vertex*>& ring_heads, int ring_id) {
    Vertex* head = ring_heads[ring_id];
    if (!head) return;
    
    // Find vertex with smallest original_vertex_id
    Vertex* smallest = head;
    Vertex* current = head->next;
    
    while (current != head && current != nullptr) {
        if (current->original_vertex_id < smallest->original_vertex_id) {
            smallest = current;
        }
        current = current->next;
    }
    
    // Update ring head
    ring_heads[ring_id] = smallest;
}


void Print_RingList(const std::map<int, Vertex*>& ring_heads, std::string file_path) {
    std::ofstream output(file_path);
    if (!output.is_open()) {
        std::cerr << "Output file path cannot open!\n";
        return;
    }
    for (const auto& [ring_id, vertex] : ring_heads) {
        const Vertex* head = vertex;
        const Vertex* cur = head;

        if (cur == nullptr) {
            continue;
        }

        do {
            output << cur->x << ", " << cur->y << "\n";

            if (cur->next == nullptr) {
                output << "Ring " << ring_id << " has a null next pointer.\n";
                break;
            }
            

            cur = cur->next;
        } while (cur != head);
    }
}
