#include "Polygon.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

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

    for (auto& [ring_id, vertices] : poly_input) {
        std::size_t sz = vertices.size();

        if (sz == 0) {
            rings_list[ring_id] = nullptr;
            continue;
        }

        if (sz == 1) {
            vertices[0].ring_id = ring_id;
            vertices[0].active = true;
            vertices[0].version = 0;
            vertices[0].prev = &vertices[0];
            vertices[0].next = &vertices[0];
            rings_list[ring_id] = &vertices[0];
            continue;
        }

        for (std::size_t id = 0; id < sz; ++id) {
            vertices[id].ring_id = ring_id;
            vertices[id].active = true;
            vertices[id].version = 0;
            vertices[id].prev = &vertices[(id + sz - 1) % sz];
            vertices[id].next = &vertices[(id + 1) % sz];
        }

        rings_list[ring_id] = &vertices[0];
    }
}

void TouchVertex(Vertex* vert) {
    if (vert != nullptr) {
        ++vert->version;
    }
}


void Print_RingList(const std::map<int, Vertex*>& rings_list, std::string file_path) {
    std::ofstream output(file_path);
    if (!output.is_open()) {
        std::cerr << "Output file path cannot open!\n";
        return;
    }
    for (const auto& [ring_id, vertex] : rings_list) {
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
