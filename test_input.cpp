#include "Helper.hpp"
#include "Polygon.hpp"
#include <iostream>

int main() {
    // Test reading the cushion input
    std::map<int, std::vector<Vertex>> poly;
    if (!Read_CSV("test_cases/input_cushion_with_hexagonal_hole.csv", poly)) {
        std::cerr << "Failed to read input" << std::endl;
        return 1;
    }
    
    std::cout << "Input rings:" << std::endl;
    for (const auto& [ring_id, vertices] : poly) {
        std::cout << "Ring " << ring_id << " (" << vertices.size() << " vertices):" << std::endl;
        for (const auto& v : vertices) {
            std::cout << "  " << v.original_vertex_id << ": (" << v.x << "," << v.y << ")" << std::endl;
        }
    }
    
    // Set up ring list
    Set_RingList(poly);
    
    std::cout << "\nRing list structure:" << std::endl;
    for (const auto& [ring_id, head] : rings_list) {
        std::cout << "Ring " << ring_id << " head: ";
        if (head) {
            std::cout << "vertex " << head->original_vertex_id << " at (" << head->x << "," << head->y << ")" << std::endl;
        } else {
            std::cout << "nullptr" << std::endl;
        }
    }
    
    return 0;
}
