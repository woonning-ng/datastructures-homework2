// CSD2183 Project 2 – Area- and Topology-Preserving Polygon Simplification

#include "Helper.hpp"
#include "Polygon.hpp"
#include "Simplifier.hpp"
#include "math.hpp"

#include <cstdio>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <vector>

std::map<int, std::vector<Vertex>> polygon;
std::map<int, Vertex*> rings_list;

static std::vector<Vertex> CollectRing(Vertex* head) {
    std::vector<Vertex> result;
    if (!head) {
        return result;
    }

    Vertex* current = head;
    do {
        result.push_back(*current);
        current = current->next;
    } while (current != head);

    return result;
}

static int CountRingVertices(Vertex* head) {
    if (!head) {
        return 0;
    }

    int count = 0;
    Vertex* current = head;
    do {
        ++count;
        current = current->next;
    } while (current != head);
    return count;
}

static double RingSignedArea(Vertex* head) {
    return SignedArea(CollectRing(head));
}

static double TotalSignedArea(const std::map<int, Vertex*>& heads) {
    double total = 0.0;
    for (const auto& [ring_id, head] : heads) {
        (void)ring_id;
        total += RingSignedArea(head);
    }
    return total;
}

static int TotalVertexCount(const std::map<int, Vertex*>& heads) {
    int total = 0;
    for (const auto& [ring_id, head] : heads) {
        (void)ring_id;
        total += CountRingVertices(head);
    }
    return total;
}

static bool SharesEndpoint(Vertex* a, Vertex* b, Vertex* c, Vertex* d) {
    return a == c || a == d || b == c || b == d;
}

static bool SegmentIntersectsExistingEdges(
    Vertex* p,
    Vertex* q,
    Vertex* skip1a,
    Vertex* skip1b,
    Vertex* skip2a,
    Vertex* skip2b,
    const std::map<int, Vertex*>& ring_heads
) {
    for (const auto& [ring_id, head] : ring_heads) {
        (void)ring_id;
        if (!head) {
            continue;
        }

        Vertex* current = head;
        do {
            Vertex* next = current->next;
            if ((current == skip1a && next == skip1b) || (current == skip1b && next == skip1a) ||
                (current == skip2a && next == skip2b) || (current == skip2b && next == skip2a)) {
                current = next;
                continue;
            }

            if (SharesEndpoint(p, q, current, next)) {
                current = next;
                continue;
            }

            if (SegmentsIntersect({p, q}, {current, next})) {
                return true;
            }

            current = next;
        } while (current != head);
    }

    return false;
}

static bool RingIsSimple(const std::vector<Vertex>& ring) {
    if (ring.size() < 3) {
        return false;
    }

    for (std::size_t i = 0; i < ring.size(); ++i) {
        Vertex a = ring[i];
        Vertex b = ring[(i + 1) % ring.size()];
        for (std::size_t j = i + 1; j < ring.size(); ++j) {
            const bool adjacent =
                j == i || (j + 1) % ring.size() == i || (i + 1) % ring.size() == j;
            if (adjacent) {
                continue;
            }

            Vertex c = ring[j];
            Vertex d = ring[(j + 1) % ring.size()];
            if (SegmentsIntersect({&a, &b}, {&c, &d})) {
                return false;
            }
        }
    }

    return true;
}

static bool RingsIntersect(const std::vector<Vertex>& lhs, const std::vector<Vertex>& rhs) {
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        Vertex a = lhs[i];
        Vertex b = lhs[(i + 1) % lhs.size()];
        for (std::size_t j = 0; j < rhs.size(); ++j) {
            Vertex c = rhs[j];
            Vertex d = rhs[(j + 1) % rhs.size()];
            if (SegmentsIntersect({&a, &b}, {&c, &d})) {
                return true;
            }
        }
    }
    return false;
}

static bool NestedTopologyIsValid(const std::map<int, std::vector<Vertex>>& rings) {
    const auto outer_it = rings.find(0);
    if (outer_it == rings.end()) {
        return false;
    }

    const std::vector<Vertex>& outer = outer_it->second;
    if (outer.size() < 3 || SignedArea(outer) <= EPSILON || !RingIsSimple(outer)) {
        return false;
    }

    for (const auto& [ring_id, ring] : rings) {
        if (ring.size() < 3 || !RingIsSimple(ring)) {
            return false;
        }

        const double area = SignedArea(ring);
        if (ring_id == 0) {
            if (area <= EPSILON) {
                return false;
            }
        } else {
            if (area >= -EPSILON) {
                return false;
            }
            if (!PointInRing(ring.front(), outer)) {
                return false;
            }
        }
    }

    for (auto lhs = rings.begin(); lhs != rings.end(); ++lhs) {
        for (auto rhs = std::next(lhs); rhs != rings.end(); ++rhs) {
            const std::vector<Vertex>& left_ring = lhs->second;
            const std::vector<Vertex>& right_ring = rhs->second;
            if (RingsIntersect(left_ring, right_ring)) {
                return false;
            }
            if (lhs->first != 0 && rhs->first != 0) {
                if (PointInRing(left_ring.front(), right_ring) ||
                    PointInRing(right_ring.front(), left_ring)) {
                    return false;
                }
            }
        }
    }

    return true;
}

static std::vector<Vertex> CollectRingAfterCollapse(Vertex* head, const CollapseCandidate& cand) {
    std::vector<Vertex> ring;
    if (!head) {
        return ring;
    }

    Vertex* current = head;
    do {
        if (current == cand.c) {
            current = current->next;
            continue;
        }

        if (current == cand.b) {
            Vertex replacement = cand.collapse.point;
            replacement.ring_id = current->ring_id;
            ring.push_back(replacement);
        } else {
            ring.push_back(*current);
        }
        current = current->next;
    } while (current != head);

    return ring;
}

static void ApplyCollapse(
    const CollapseCandidate& cand,
    std::map<int, Vertex*>& ring_heads
) {
    Vertex* a = cand.a;
    Vertex* b = cand.b;
    Vertex* c = cand.c;
    Vertex* d = cand.d;
    const Vertex& e = cand.collapse.point;

    b->x = e.x;
    b->y = e.y;
    b->next = d;
    d->prev = b;

    c->active = false;
    c->prev = nullptr;
    c->next = nullptr;

    if (ring_heads[a->ring_id] == c) {
        ring_heads[a->ring_id] = b;
    }

    TouchVertex(a);
    TouchVertex(b);
    TouchVertex(d);
    if (a->prev) {
        TouchVertex(a->prev);
    }
    if (d->next) {
        TouchVertex(d->next);
    }
}

static bool IsCollapseGeometricallyValid(
    const CollapseCandidate& cand,
    const std::map<int, Vertex*>& ring_heads
) {
    Vertex e = cand.collapse.point;
    e.ring_id = cand.a->ring_id;

    if (SegmentIntersectsExistingEdges(cand.a, &e, cand.a, cand.b, cand.b, cand.c, ring_heads)) {
        return false;
    }
    if (SegmentIntersectsExistingEdges(&e, cand.d, cand.b, cand.c, cand.c, cand.d, ring_heads)) {
        return false;
    }

    std::map<int, std::vector<Vertex>> simulated_rings;
    for (const auto& [ring_id, head] : ring_heads) {
        if (ring_id == cand.a->ring_id) {
            simulated_rings[ring_id] = CollectRingAfterCollapse(head, cand);
        } else {
            simulated_rings[ring_id] = CollectRing(head);
        }
    }
    return NestedTopologyIsValid(simulated_rings);
}

static void PrintCoord(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%g", value);
    std::printf("%s", buffer);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <input_file.csv> <target_vertices>\n", argv[0]);
        return 1;
    }

    const std::string input_file = argv[1];
    const int target_vertices = std::stoi(argv[2]);

    if (!Read_CSV(input_file, polygon)) {
        return 1;
    }

    Set_RingList(polygon);

    const double input_area = TotalSignedArea(rings_list);
    int current_vertices = TotalVertexCount(rings_list);
    CollapseQueue queue = BuildInitialCollapseQueue(rings_list);
    double total_displacement = 0.0;

    while (current_vertices > target_vertices && !queue.empty()) {
        std::optional<CollapseCandidate> best = PopBestValidCandidate(queue);
        if (!best.has_value()) {
            break;
        }

        const int ring_id = best->a->ring_id;
        if (CountRingVertices(rings_list[ring_id]) <= 3) {
            continue;
        }

        if (!IsCollapseGeometricallyValid(*best, rings_list)) {
            continue;
        }

        total_displacement += best->collapse.local_displacement;
        ApplyCollapse(*best, rings_list);
        --current_vertices;

        queue = BuildInitialCollapseQueue(rings_list);
    }

    std::printf("ring_id,vertex_id,x,y\n");
    for (const auto& [ring_id, head] : rings_list) {
        std::vector<Vertex> ring = CollectRing(head);
        for (int vertex_id = 0; vertex_id < static_cast<int>(ring.size()); ++vertex_id) {
            std::printf("%d,%d,", ring_id, vertex_id);
            PrintCoord(ring[vertex_id].x);
            std::printf(",");
            PrintCoord(ring[vertex_id].y);
            std::printf("\n");
        }
    }

    const double output_area = TotalSignedArea(rings_list);
    std::printf("Total signed area in input: %e\n", input_area);
    std::printf("Total signed area in output: %e\n", output_area);
    std::printf("Total areal displacement: %e\n", total_displacement);
    return 0;
}
