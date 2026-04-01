// File containing the definitions of various helper math functions
// that will be used in computations

#include "math.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace {

double SignedDoubleArea(const std::vector<Vertex>& vertices) {
    if (vertices.size() < 3) {
        return 0.0;
    }

    double area = 0.0;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        area += Determinant(vertices[i], vertices[(i + 1) % vertices.size()]);
    }
    return area;
}

double Cross(Vertex origin, Vertex lhs, Vertex rhs) {
    return CrossProduct(lhs - origin, rhs - origin);
}

bool NearlyEqual(double lhs, double rhs, double epsilon = EPSILON) {
    return std::abs(lhs - rhs) <= epsilon;
}

bool SamePoint(const Vertex& lhs, const Vertex& rhs, double epsilon = EPSILON) {
    return NearlyEqual(lhs.x, rhs.x, epsilon) && NearlyEqual(lhs.y, rhs.y, epsilon);
}

bool IsFinitePoint(const Vertex& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

std::optional<Vertex> InfiniteLineIntersection(
    const Vertex& p1,
    const Vertex& p2,
    const Vertex& q1,
    const Vertex& q2
) {
    const Vertex r = p2 - p1;
    const Vertex s = q2 - q1;
    const double denominator = CrossProduct(r, s);
    if (std::abs(denominator) <= EPSILON) {
        return std::nullopt;
    }

    const double t = CrossProduct(q1 - p1, s) / denominator;
    return p1 + (r * t);
}

std::vector<Vertex> NormalizedCCWPolygon(std::vector<Vertex> vertices) {
    if (SignedDoubleArea(vertices) < 0.0) {
        std::reverse(vertices.begin(), vertices.end());
    }
    return vertices;
}

bool InsideHalfPlane(const Vertex& edge_start, const Vertex& edge_end, const Vertex& point) {
    return Cross(edge_start, edge_end, point) >= -EPSILON;
}

std::vector<Vertex> ConvexClip(std::vector<Vertex> subject, const std::vector<Vertex>& clip_polygon) {
    subject = NormalizedCCWPolygon(std::move(subject));
    const std::vector<Vertex> clip = NormalizedCCWPolygon(clip_polygon);

    for (std::size_t i = 0; i < clip.size() && !subject.empty(); ++i) {
        const Vertex& clip_start = clip[i];
        const Vertex& clip_end = clip[(i + 1) % clip.size()];

        std::vector<Vertex> output;
        for (std::size_t j = 0; j < subject.size(); ++j) {
            const Vertex& current = subject[j];
            const Vertex& previous = subject[(j + subject.size() - 1) % subject.size()];
            const bool current_inside = InsideHalfPlane(clip_start, clip_end, current);
            const bool previous_inside = InsideHalfPlane(clip_start, clip_end, previous);

            if (current_inside) {
                if (!previous_inside) {
                    const std::optional<Vertex> intersection =
                        InfiniteLineIntersection(previous, current, clip_start, clip_end);
                    if (intersection.has_value()) {
                        output.push_back(*intersection);
                    }
                }
                output.push_back(current);
            } else if (previous_inside) {
                const std::optional<Vertex> intersection =
                    InfiniteLineIntersection(previous, current, clip_start, clip_end);
                if (intersection.has_value()) {
                    output.push_back(*intersection);
                }
            }
        }

        subject = std::move(output);
    }

    std::vector<Vertex> deduplicated;
    for (const Vertex& point : subject) {
        if (deduplicated.empty() || !SamePoint(point, deduplicated.back())) {
            deduplicated.push_back(point);
        }
    }
    if (deduplicated.size() >= 2 && SamePoint(deduplicated.front(), deduplicated.back())) {
        deduplicated.pop_back();
    }
    return deduplicated;
}

double TriangleSymmetricDifferenceArea(
    const Vertex& a1,
    const Vertex& b1,
    const Vertex& c1,
    const Vertex& a2,
    const Vertex& b2,
    const Vertex& c2
) {
    const std::vector<Vertex> triangle_one{a1, b1, c1};
    const std::vector<Vertex> triangle_two{a2, b2, c2};

    const double area_one = std::abs(SignedDoubleArea(triangle_one)) * 0.5;
    const double area_two = std::abs(SignedDoubleArea(triangle_two)) * 0.5;
    const std::vector<Vertex> intersection = ConvexClip(triangle_one, triangle_two);
    const double intersection_area =
        intersection.size() >= 3 ? std::abs(SignedDoubleArea(intersection)) * 0.5 : 0.0;
    return area_one + area_two - (2.0 * intersection_area);
}

bool IsStrictlyBetter(const APSCCollapseResult& lhs, const APSCCollapseResult& rhs) {
    if (!NearlyEqual(lhs.local_displacement, rhs.local_displacement)) {
        return lhs.local_displacement < rhs.local_displacement;
    }
    return static_cast<int>(lhs.side) < static_cast<int>(rhs.side);
}

} // namespace

double Determinant(Vertex A, Vertex B) {
    return A.x * B.y - A.y * B.x;
}

double Determinant(Line line) {
    return line.a->x * line.b->y - line.a->y * line.b->x;
}

double DotProduct(Vector A, Vector B) {
    return A.x * B.x + A.y * B.y;
}

double CrossProduct(Vector A, Vector B) {
    return A.x * B.y - A.y * B.x;
}

std::optional<Vertex> LineIntersection(Line A, Line B) {
    if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) {
        return std::nullopt;
    }
    return InfiniteLineIntersection(*A.a, *A.b, *B.a, *B.b);
}

double SignedArea(std::vector<Vertex> poly, bool reverse) {
    std::vector<Vertex> vertices = std::move(poly);
    if (reverse) {
        std::reverse(vertices.begin(), vertices.end());
    }
    return SignedDoubleArea(vertices) * 0.5;
}

double TriangleArea(Vertex A, Vertex B, Vertex C) {
    return CrossProduct(B - A, C - A) / 2.0;
}

bool SegmentsIntersect(Line A, Line B) {
    if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) {
        return false;
    }

    const int d1 = Orientation(*B.a, *B.b, *A.a);
    const int d2 = Orientation(*B.a, *B.b, *A.b);
    const int d3 = Orientation(*A.a, *A.b, *B.a);
    const int d4 = Orientation(*A.a, *A.b, *B.b);

    if (d1 != d2 && d3 != d4) {
        return true;
    }

    if (d1 == 0 && PointOnSegment(*A.a, B)) return true;
    if (d2 == 0 && PointOnSegment(*A.b, B)) return true;
    if (d3 == 0 && PointOnSegment(*B.a, A)) return true;
    if (d4 == 0 && PointOnSegment(*B.b, A)) return true;

    return false;
}

bool IsCollinear(Vertex A, Vertex B, Vertex C) {
    return Orientation(A, B, C) == 0;
}

bool PointOnSegment(Vertex P, Line seg) {
    if (seg.a == nullptr || seg.b == nullptr) {
        return false;
    }
    if (!IsCollinear(*seg.a, P, *seg.b)) {
        return false;
    }

    return std::min(seg.a->x, seg.b->x) - EPSILON <= P.x &&
           P.x <= std::max(seg.a->x, seg.b->x) + EPSILON &&
           std::min(seg.a->y, seg.b->y) - EPSILON <= P.y &&
           P.y <= std::max(seg.a->y, seg.b->y) + EPSILON;
}

int Orientation(Vertex A, Vertex B, Vertex C) {
    const double cross = CrossProduct(
        {B.x - A.x, B.y - A.y},
        {C.x - A.x, C.y - A.y}
    );
    if (std::abs(cross) < EPSILON) {
        return 0;
    }
    return (cross > 0) ? 1 : -1;
}

std::optional<Vertex> ComputeAreaPreservingPointE(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D,
    APSCPlacementSide side
) {
    const double signed_double_local_area = SignedDoubleArea({A, B, C, D});
    const Vertex ad = D - A;

    Vertex direction;
    Vertex anchor;

    if (side == APSCPlacementSide::OnAB) {
        direction = B - A;
        anchor = A;
    } else {
        direction = C - D;
        anchor = D;
    }

    const double denominator = CrossProduct(direction, ad);
    if (std::abs(denominator) <= EPSILON) {
        return std::nullopt;
    }

    const double t = signed_double_local_area / denominator;
    const Vertex candidate = anchor + (direction * t);

    if (!IsFinitePoint(candidate) || SamePoint(candidate, A) || SamePoint(candidate, D)) {
        return std::nullopt;
    }

    return candidate;
}

double ComputeLocalArealDisplacement(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D,
    const Vertex& E,
    APSCPlacementSide side
) {
    if (side == APSCPlacementSide::OnAB) {
        return TriangleSymmetricDifferenceArea(B, C, D, B, E, D);
    }
    return TriangleSymmetricDifferenceArea(A, B, C, A, E, C);
}

std::optional<APSCCollapseResult> ComputeBestAPSCReplacement(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D
) {
    std::optional<APSCCollapseResult> best;

    for (APSCPlacementSide side : {APSCPlacementSide::OnAB, APSCPlacementSide::OnCD}) {
        const std::optional<Vertex> point = ComputeAreaPreservingPointE(A, B, C, D, side);
        if (!point.has_value()) {
            continue;
        }

        APSCCollapseResult candidate;
        candidate.point = *point;
        candidate.local_displacement = ComputeLocalArealDisplacement(A, B, C, D, *point, side);
        candidate.side = side;

        if (!best.has_value() || IsStrictlyBetter(candidate, *best)) {
            best = candidate;
        }
    }

    return best;
}

const char* APSCPlacementSideName(APSCPlacementSide side) {
    switch (side) {
        case APSCPlacementSide::OnAB:
            return "AB";
        case APSCPlacementSide::OnCD:
            return "CD";
        default:
            return "unknown";
    }
}

double DistancePointToSegment(Vertex P, Line seg) {
    if (seg.a == nullptr || seg.b == nullptr) {
        return std::numeric_limits<double>::infinity();
    }

    const Vector ab = *seg.b - *seg.a;
    const Vector ap = P - *seg.a;
    const double denom = DotProduct(ab, ab);
    if (denom <= EPSILON) {
        return DistancePointToPoint(P, *seg.a);
    }

    const double t = std::clamp(DotProduct(ap, ab) / denom, 0.0, 1.0);
    const Vertex projection = *seg.a + (ab * t);
    return DistancePointToPoint(P, projection);
}

double DistancePointToPoint(Vertex A, Vertex B) {
    const double dx = A.x - B.x;
    const double dy = A.y - B.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool PointInRing(Vertex P, std::vector<Vertex> ring) {
    if (ring.size() < 3) {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
        Line edge{&ring[j], &ring[i]};
        if (PointOnSegment(P, edge)) {
            return true;
        }

        const bool intersects =
            ((ring[i].y > P.y) != (ring[j].y > P.y)) &&
            (P.x < (ring[j].x - ring[i].x) * (P.y - ring[i].y) / (ring[j].y - ring[i].y) + ring[i].x);

        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

int WindingNumber(Vertex P, std::vector<Vertex> ring) {
    if (ring.size() < 3) {
        return 0;
    }

    int winding_number = 0;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        Vertex current = ring[i];
        Vertex next = ring[(i + 1) % ring.size()];
        Line edge{&current, &next};
        if (PointOnSegment(P, edge)) {
            return 1;
        }

        if (current.y <= P.y) {
            if (next.y > P.y && Orientation(current, next, P) > 0) {
                ++winding_number;
            }
        } else if (next.y <= P.y && Orientation(current, next, P) < 0) {
            --winding_number;
        }
    }

    return winding_number;
}

bool IsRingClockwise(std::vector<Vertex> ring) {
    return SignedArea(std::move(ring)) < 0.0;
}
