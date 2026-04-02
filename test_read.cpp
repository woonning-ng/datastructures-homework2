#include "Helper.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.csv>\n";
        return 1;
    }
    
    std::map<int, std::vector<Vertex>> polygon;
    if (!Read_CSV(argv[1], polygon)) {
        std::cerr << "Failed to read CSV\n";
        return 1;
    }
    
    std::cout << "Successfully read CSV with " << polygon.size() << " rings:\n";
    for (const auto& [ring_id, vertices] : polygon) {
        std::cout << "Ring " << ring_id << ": " << vertices.size() << " vertices\n";
        for (const Vertex& v : vertices) {
            std::cout << "  (" << v.x << ", " << v.y << ")\n";
        }
    }
    
    return 0;
}
