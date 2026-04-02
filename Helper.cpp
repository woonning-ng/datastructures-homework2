#include "Helper.hpp"

#include "math.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

std::string Trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

bool SamePoint(const Vertex& lhs, const Vertex& rhs) {
    return std::abs(lhs.x - rhs.x) <= EPSILON && std::abs(lhs.y - rhs.y) <= EPSILON;
}

} // namespace

int Read_CSV(const std::string& file_path, std::map<int, std::vector<Vertex>>& poly) {
    poly.clear();

    std::ifstream input(file_path);
    if (!input.is_open()) {
        std::cerr << "No such path exists\n";
        return 0;
    }

    std::string line;
    std::getline(input, line);

    while (std::getline(input, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }

        std::stringstream row(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(row, field, ',')) {
            fields.push_back(Trim(field));
        }

        if (fields.size() != 4) {
            std::cerr << "Malformed CSV row: " << line << "\n";
            return 0;
        }

        try {
            const int ring_id = std::stoi(fields[0]);
            const int vertex_id = std::stoi(fields[1]);
            Vertex vertex;
            vertex.x = std::stod(fields[2]);
            vertex.y = std::stod(fields[3]);
            vertex.original_vertex_id = vertex_id;
            poly[ring_id].push_back(vertex);
        } catch (const std::exception&) {
            std::cerr << "Malformed CSV row: " << line << "\n";
            return 0;
        }
    }

    for (auto& [ring_id, vertices] : poly) {
        (void)ring_id;
        if (vertices.size() >= 2 && SamePoint(vertices.front(), vertices.back())) {
            vertices.pop_back();
        }
    }

    return 1;
}

void Print_Poly(const std::string& file_path, const std::map<int, std::vector<Vertex>>& poly) {
    std::ofstream output(file_path);
    if (!output.is_open()) {
        std::cerr << "Output Check File cannot be opened!!\n";
        return;
    }

    for (const auto& [ring_id, ring] : poly) {
        for (std::size_t i = 0; i < ring.size(); ++i) {
            output << ring_id << "," << i << "," << ring[i].x << "," << ring[i].y << "\n";
        }
    }
}

std::vector<Vertex> NormalizeRingForOutput(Vertex* head) {
    std::vector<Vertex> ring;
    if (!head) {
        return ring;
    }

    // Collect all vertices in their current order
    Vertex* current = head;
    do {
        ring.push_back(*current);
        current = current->next;
    } while (current != head);

    if (ring.empty()) {
        return ring;
    }

    // Remove duplicate closing vertex if present
    if (ring.size() >= 2 && SamePoint(ring.front(), ring.back())) {
        ring.pop_back();
    }
    
    return ring;
}