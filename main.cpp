// CSD2183 Project 2 – Area- and Topology-Preserving Polygon Simplification

#include "Helper.hpp"
#include "Polygon.hpp"
#include "Simplifier.hpp"
#include "SpatialGrid.hpp"
#include "math.hpp"

#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

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

static bool PointEq(const Vertex& a, const Vertex& b) {
    return std::abs(a.x - b.x) <= 1e-12 && std::abs(a.y - b.y) <= 1e-12;
}

static int OrientSign(const Vertex& a, const Vertex& b, const Vertex& c) {
    const long double abx = static_cast<long double>(b.x) - static_cast<long double>(a.x);
    const long double aby = static_cast<long double>(b.y) - static_cast<long double>(a.y);
    const long double acx = static_cast<long double>(c.x) - static_cast<long double>(a.x);
    const long double acy = static_cast<long double>(c.y) - static_cast<long double>(a.y);
    const long double value = abx * acy - aby * acx;
    const long double tol =
        1e-12L * (fabsl(abx) + fabsl(aby) + fabsl(acx) + fabsl(acy) + 1.0L);
    if (fabsl(value) <= tol) {
        return 0;
    }
    return value > 0 ? 1 : -1;
}

static bool OnSegment(const Vertex& a, const Vertex& b, const Vertex& p) {
    if (OrientSign(a, b, p) != 0) {
        return false;
    }
    const double eps = 1e-12;
    return p.x >= std::min(a.x, b.x) - eps && p.x <= std::max(a.x, b.x) + eps &&
           p.y >= std::min(a.y, b.y) - eps && p.y <= std::max(a.y, b.y) + eps;
}

static bool CollinearOverlapNontrivial(
    const Vertex& a,
    const Vertex& b,
    const Vertex& c,
    const Vertex& d
) {
    if (OrientSign(a, b, c) != 0 || OrientSign(a, b, d) != 0) {
        return false;
    }
    const double eps = 1e-12;
    const double abx = std::abs(b.x - a.x);
    const double aby = std::abs(b.y - a.y);
    auto proj = [&](const Vertex& p) { return (abx >= aby) ? p.x : p.y; };
    double a0 = proj(a), a1 = proj(b);
    double c0 = proj(c), c1 = proj(d);
    if (a0 > a1) {
        std::swap(a0, a1);
    }
    if (c0 > c1) {
        std::swap(c0, c1);
    }
    const double overlap = std::min(a1, c1) - std::max(a0, c0);
    return overlap > eps;
}

static bool SegmentsIntersectNontrivial(
    const Vertex& p1,
    const Vertex& p2,
    const Vertex& p3,
    const Vertex& p4,
    bool ignore_shared_endpoints
) {
    const bool shared_endpoint =
        PointEq(p1, p3) || PointEq(p1, p4) || PointEq(p2, p3) || PointEq(p2, p4);

    if (CollinearOverlapNontrivial(p1, p2, p3, p4)) {
        return true;
    }

    const int o1 = OrientSign(p1, p2, p3);
    const int o2 = OrientSign(p1, p2, p4);
    const int o3 = OrientSign(p3, p4, p1);
    const int o4 = OrientSign(p3, p4, p2);

    if (o1 * o2 < 0 && o3 * o4 < 0) {
        return true;
    }

    const bool intersects =
        (o1 == 0 && OnSegment(p1, p2, p3)) ||
        (o2 == 0 && OnSegment(p1, p2, p4)) ||
        (o3 == 0 && OnSegment(p3, p4, p1)) ||
        (o4 == 0 && OnSegment(p3, p4, p2));

    if (!intersects) {
        return false;
    }
    if (ignore_shared_endpoints && shared_endpoint) {
        return false;
    }
    return true;
}

static bool RingContainsPoint(Vertex* head, const Vertex& point) {
    if (!head) {
        return false;
    }

    Vertex* current = head;
    do {
        if (current->active && PointEq(*current, point)) {
            return true;
        }
        current = current->next;
    } while (current != head);

    return false;
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

static Vertex* ApplyCollapse(
    const CollapseCandidate& cand,
    std::map<int, Vertex*>& ring_heads,
    std::map<int, int>& next_generated_id,
    SpatialGrid* grid
) {
    Vertex* a = cand.a;
    Vertex* b = cand.b;
    Vertex* c = cand.c;
    Vertex* d = cand.d;
    const Vertex& e = cand.collapse.point;

    if (!a || !b || !c || !d) {
        return nullptr;
    }

    const int ring_id = a->ring_id;
    
    // Update ring head if we're removing the current head
    Vertex* current_head = ring_heads[ring_id];
    if (current_head == b || current_head == c) {
        ring_heads[ring_id] = d;
    }

    if (grid) {
        grid->Remove(a);
        grid->Remove(b);
        grid->Remove(c);
    }

    const int e_id = next_generated_id[ring_id]++;
    Vertex* e_vertex = AllocateVertex(e.x, e.y, e_id, ring_id);

    Delete_Vertex(b);
    Delete_Vertex(c);
    Insert_Vertex(a, e_vertex);
    
    // d's prev pointer changed from c to e_vertex
    TouchVertex(d);

    if (grid) {
        grid->Insert(a, ring_id);
        grid->Insert(e_vertex, ring_id);
    }
    return e_vertex;
}

static bool CollapseCausesIntersection(
    Vertex* ring_head,
    Vertex* a,
    Vertex* b,
    Vertex* c,
    Vertex* d,
    const Vertex& e,
    const SpatialGrid* grid
) {
    if (!ring_head || !a || !b || !c || !d) {
        return true;
    }

    if (PointEq(*a, e) || PointEq(*d, e)) {
        return true;
    }

    if (grid) {
        bool point_hit = false;
        grid->Query(e.x, e.y, e.x, e.y, [&](Vertex* u, int rid) -> bool {
            if (rid != a->ring_id) {
                return true;
            }
            Vertex* w = u->next;
            if (!w) {
                return true;
            }
            if ((PointEq(*u, e) && u != a && u != d) || (PointEq(*w, e) && w != a && w != d)) {
                point_hit = true;
                return false;
            }
            return true;
        });
        if (point_hit) {
            return true;
        }
    } else {
        if (RingContainsPoint(ring_head, e)) {
            return true;
        }
    }

    if (grid) {
        const double x0 = std::min({a->x, e.x, d->x});
        const double y0 = std::min({a->y, e.y, d->y});
        const double x1 = std::max({a->x, e.x, d->x});
        const double y1 = std::max({a->y, e.y, d->y});
        bool found = false;
        grid->Query(x0, y0, x1, y1, [&](Vertex* u, int rid) -> bool {
            if (rid != a->ring_id) {
                return true;
            }
            Vertex* w = u->next;
            if (!w) {
                return true;
            }
            if (u == b || u == c || w == b || w == c) {
                return true;
            }
            if (SegmentsIntersectNontrivial(*a, e, *u, *w, true)) {
                found = true;
                return false;
            }
            if (SegmentsIntersectNontrivial(e, *d, *u, *w, true)) {
                found = true;
                return false;
            }
            return true;
        });
        if (found) {
            return true;
        }
    } else {
        Vertex* u = ring_head;
        do {
            Vertex* const w = u->next;
            if (!w) {
                return true;
            }
            if (u == b || u == c || w == b || w == c) {
                u = w;
                continue;
            }
            if (SegmentsIntersectNontrivial(*a, e, *u, *w, true)) {
                return true;
            }
            if (SegmentsIntersectNontrivial(e, *d, *u, *w, true)) {
                return true;
            }
            u = w;
        } while (u != ring_head);
    }

    return false;
}

static bool CollapseCausesCrossRingIntersection(
    const std::map<int, Vertex*>& ring_heads,
    int ring_id,
    Vertex* a,
    Vertex* b,
    Vertex* c,
    Vertex* d,
    const Vertex& e,
    const SpatialGrid* grid
) {
    Vertex* const ring_head = ring_heads.at(ring_id);
    if (CollapseCausesIntersection(ring_head, a, b, c, d, e, grid)) {
        return true;
    }

    if (ring_heads.size() <= 1) {
        return false;
    }

    // Collect current ring to determine if it's a hole
    std::vector<Vertex> current_ring = CollectRing(ring_head);
    const double current_area = SignedArea(current_ring);
    const bool current_is_outer = current_area > EPSILON;
    const bool current_is_hole = current_area < -EPSILON;

    for (const auto& [other_ring_id, other_head] : ring_heads) {
        if (other_ring_id == ring_id || !other_head) {
            continue;
        }
        
        // Collect other ring vertices
        std::vector<Vertex> other_ring = CollectRing(other_head);
        const double other_area = SignedArea(other_ring);
        const bool other_is_outer = other_area > EPSILON;
        const bool other_is_hole = other_area < -EPSILON;
        
        // Check if E is inside the other ring
        const bool e_inside_other = PointInRing(e, other_ring);
        
        // Topology rules:
        // 1. Outer ring points must NOT be inside any hole
        // 2. Hole points must be INSIDE the outer ring
        // 3. Holes must not contain points from other holes
        // 4. Rings must not intersect (handled by edge intersection checks)
        
        if (other_is_outer) {
            if (current_is_hole && !e_inside_other) {
                return true;  // Hole moved outside outer ring
            }
            if (current_is_outer && e_inside_other) {
                return true;  // Outer ring point moved inside itself
            }
        }
        
        if (other_is_hole) {
            if (e_inside_other) {
                return true;  // Point moved inside a hole (invalid for both outer and holes)
            }
        }
        
        // Check if E is exactly on any vertex of other rings
        for (const Vertex& v : other_ring) {
            if (PointEq(v, e)) {
                return true;
            }
        }
    }

    // Check edge intersections with other rings (using grid for efficiency)
    if (grid) {
        const double x0 = std::min({a->x, e.x, d->x});
        const double y0 = std::min({a->y, e.y, d->y});
        const double x1 = std::max({a->x, e.x, d->x});
        const double y1 = std::max({a->y, e.y, d->y});

        bool found = false;
        grid->Query(x0, y0, x1, y1, [&](Vertex* u, int rid) -> bool {
            if (rid == ring_id) {
                return true;
            }
            Vertex* w = u->next;
            if (!w) {
                return true;
            }
            
            // Skip edges that share endpoints with our new edges
            if (u == a || u == d || w == a || w == d) {
                return true;
            }
            
            if (SegmentsIntersectNontrivial(*a, e, *u, *w, false)) {
                found = true;
                return false;
            }
            if (SegmentsIntersectNontrivial(e, *d, *u, *w, false)) {
                found = true;
                return false;
            }
            return true;
        });
        if (found) {
            return true;
        }
    } else {
        // Non-grid fallback
        for (const auto& [other_ring_id, other_head] : ring_heads) {
            if (other_ring_id == ring_id || !other_head) {
                continue;
            }
            
            Vertex* u = other_head;
            do {
                Vertex* const w = u->next;
                if (!w) {
                    return true;
                }
                if (u == a || u == d || w == a || w == d) {
                    u = w;
                    continue;
                }
                if (SegmentsIntersectNontrivial(*a, e, *u, *w, false)) {
                    return true;
                }
                if (SegmentsIntersectNontrivial(e, *d, *u, *w, false)) {
                    return true;
                }
                u = w;
            } while (u != other_head);
        }
    }

    return false;
}

static bool PointOnRingBoundary(const Vertex& P, const std::vector<Vertex>& ring) {
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const Vertex& a = ring[i];
        const Vertex& b = ring[(i + 1) % ring.size()];
        Line edge{const_cast<Vertex*>(&a), const_cast<Vertex*>(&b)};
        if (PointOnSegment(P, edge)) {
            return true;
        }
    }
    return false;
}


static bool IsCollapseGeometricallyValid(
    const CollapseCandidate& cand,
    const std::map<int, Vertex*>& ring_heads,
    const SpatialGrid* grid
) {
    if (!cand.a || !cand.b || !cand.c || !cand.d) {
        return false;
    }

    Vertex replacement = cand.collapse.point;
    replacement.ring_id = cand.a->ring_id;

    return !CollapseCausesCrossRingIntersection(
        ring_heads,
        cand.a->ring_id,
        cand.a,
        cand.b,
        cand.c,
        cand.d,
        replacement,
        grid
    );
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

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

    SpatialGrid grid;
    grid.Build(rings_list);

    std::map<int, int> next_generated_id;
    for (const auto& [ring_id, vertices] : polygon) {
        int max_id = -1;
        for (const Vertex& vertex : vertices) {
            max_id = std::max(max_id, vertex.original_vertex_id);
        }
        next_generated_id[ring_id] = max_id + 1;
    }

    const double input_area = TotalSignedArea(rings_list);
    int current_vertices = TotalVertexCount(rings_list);
    CollapseQueue queue = BuildInitialCollapseQueue(rings_list);
    double total_displacement = 0.0;

    while (current_vertices > target_vertices && !queue.empty()) {
        std::optional<CollapseCandidate> best = PopBestValidCandidate(queue);
        if (!best.has_value()) {
            break;
        }

        if (!IsCollapseGeometricallyValid(*best, rings_list, &grid)) {
            continue;
        }

        total_displacement += best->collapse.local_displacement;
        Vertex* queue_e = ApplyCollapse(*best, rings_list, next_generated_id, &grid);
        --current_vertices;

        QueueLocalCandidatesAround(queue, queue_e);
    }



    for (const auto& [ring_id, head] : rings_list) {
        (void)head;
        ResetRingHeadToSmallestId(rings_list, ring_id);
    }

    std::cout << std::defaultfloat << std::setprecision(10);
    std::cout << "ring_id,vertex_id,x,y\n";
    for (const auto& [ring_id, head] : rings_list) {
        if (!head) {
            continue;
        }
        
        // Normalize the ring before output
        std::vector<Vertex> normalized = NormalizeRingForOutput(head);
        
        // Use sequential vertex IDs starting from 0 as expected by test cases
        for (std::size_t i = 0; i < normalized.size(); ++i) {
            std::cout << ring_id << ',' << i << ','
                    << normalized[i].x << ',' << normalized[i].y << '\n';
        }
    }

    const double output_area = TotalSignedArea(rings_list);
    {
        char buf[128];
        // Use scientific notation with 6 decimal places like test files
        std::snprintf(buf, sizeof(buf), "Total signed area in input: %.6e", input_area);
        std::cout << buf << '\n';
        std::snprintf(buf, sizeof(buf), "Total signed area in output: %.6e", output_area);
        std::cout << buf << '\n';
        std::snprintf(buf, sizeof(buf), "Total areal displacement: %.6e", total_displacement);
        std::cout << buf << '\n';
    }
    return 0;
}
