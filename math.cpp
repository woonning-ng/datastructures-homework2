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

namespace {

Vertex IntersectEStarWithLine(double a, double b, double c, const Vertex& p1, const Vertex& p2) {
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double denom = a * dx + b * dy;
    if (std::abs(denom) < 1e-15) {
        const Vertex mid{(p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5};
        const double n2 = a * a + b * b;
        const double val = a * mid.x + b * mid.y + c;
        return {mid.x - a * val / n2, mid.y - b * val / n2};
    }
    const double t = -(a * p1.x + b * p1.y + c) / denom;
    return {p1.x + t * dx, p1.y + t * dy};
}

double TriArea(const Vertex& p1, const Vertex& p2, const Vertex& p3) {
    return 0.5 * ((p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y));
}

double PolyArea(const std::vector<Vertex>& pts) {
    if (pts.size() < 3) {
        return 0.0;
    }
    long double acc = 0.0L;
    for (std::size_t i = 0, n = pts.size(); i < n; ++i) {
        const Vertex& a = pts[i];
        const Vertex& b = pts[(i + 1) % n];
        acc += static_cast<long double>(a.x) * static_cast<long double>(b.y) -
               static_cast<long double>(b.x) * static_cast<long double>(a.y);
    }
    return std::abs(static_cast<double>(acc * 0.5L));
}

bool SegIntersectParams(
    const Vertex& p1,
    const Vertex& p2,
    const Vertex& p3,
    const Vertex& p4,
    double& t,
    double& s
) {
    const double dx12 = p2.x - p1.x;
    const double dy12 = p2.y - p1.y;
    const double dx34 = p4.x - p3.x;
    const double dy34 = p4.y - p3.y;
    const double denom = dx12 * dy34 - dy12 * dx34;
    if (std::abs(denom) < 1e-15) {
        return false;
    }
    const double dx13 = p3.x - p1.x;
    const double dy13 = p3.y - p1.y;
    t = (dx13 * dy34 - dy13 * dx34) / denom;
    s = (dx13 * dy12 - dy13 * dx12) / denom;
    const double eps = 1e-9;
    return t >= -eps && t <= 1.0 + eps && s >= -eps && s <= 1.0 + eps;
}

} // namespace

double ComputeProject2ArealDisplacement(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D,
    const Vertex& E
) {
    double t = 0.0;
    double s = 0.0;
    const double eps_cross = 1e-10;

    if (SegIntersectParams(E, D, B, C, t, s)) {
        if (!(t <= eps_cross || t >= 1.0 - eps_cross || s <= eps_cross || s >= 1.0 - eps_cross)) {
            const Vertex I{E.x + t * (D.x - E.x), E.y + t * (D.y - E.y)};
            const double a1 = TriArea(A, B, I) + TriArea(A, I, E);
            const double a2 = TriArea(I, C, D) + TriArea(I, D, E);
            return std::abs(a1) + std::abs(a2);
        }
    }

    if (SegIntersectParams(A, E, B, C, t, s)) {
        if (!(t <= eps_cross || t >= 1.0 - eps_cross || s <= eps_cross || s >= 1.0 - eps_cross)) {
            const Vertex I{A.x + t * (E.x - A.x), A.y + t * (E.y - A.y)};
            const double a1 = std::abs(TriArea(A, B, I));
            const double a2 = std::abs(TriArea(I, C, D) + TriArea(I, D, E));
            return a1 + a2;
        }
    }

    if (SegIntersectParams(E, D, A, B, t, s)) {
        if (!(t <= eps_cross || t >= 1.0 - eps_cross || s <= eps_cross || s >= 1.0 - eps_cross)) {
            const Vertex I{E.x + t * (D.x - E.x), E.y + t * (D.y - E.y)};
            const double a1 = std::abs(TriArea(A, I, E));
            const double a2 = PolyArea({I, B, C, D, E});
            return a1 + a2;
        }
    }

    if (SegIntersectParams(A, E, C, D, t, s)) {
        if (!(t <= eps_cross || t >= 1.0 - eps_cross || s <= eps_cross || s >= 1.0 - eps_cross)) {
            const Vertex J{A.x + t * (E.x - A.x), A.y + t * (E.y - A.y)};
            const double a1 = std::abs(TriArea(J, D, E));
            const double a2 = PolyArea({A, B, C, J, E});
            return a1 + a2;
        }
    }

    const double area = Determinant(A, B) +
                        Determinant(B, C) +
                        Determinant(C, D) +
                        Determinant(D, E) +
                        Determinant(E, A);
    const double shoelace_disp = std::abs(area) * 0.5;
    const double alt1 = std::abs(TriArea(A, B, C) + TriArea(A, C, E)) + std::abs(TriArea(C, D, E));
    const double alt2 = std::abs(TriArea(A, B, E)) +
                        std::abs(TriArea(B, C, D) + TriArea(B, D, E));
    const double alt3 = std::abs(TriArea(A, B, E)) + std::abs(TriArea(E, C, D));
    return std::max(std::max(shoelace_disp, alt1), std::max(alt2, alt3));
}

Vertex ComputeProject2APSCPointE(
    const Vertex& A,
    const Vertex& B,
    const Vertex& C,
    const Vertex& D
) {
    const double a = D.y - A.y;
    const double b = A.x - D.x;
    const double c = -B.y * A.x +
                     (A.y - C.y) * B.x +
                     (B.y - D.y) * C.x +
                     C.y * D.x;

    const double adx = D.x - A.x;
    const double ady = D.y - A.y;
    const double ad2 = adx * adx + ady * ady;
    if (ad2 < 1e-18) {
        return {(B.x + C.x) * 0.5, (B.y + C.y) * 0.5};
    }

    const double val_a = a * A.x + b * A.y + c;
    const double scale = std::abs(a) + std::abs(b) + 1.0;
    if (std::abs(val_a) < 1e-10 * scale) {
        return {(A.x + D.x) * 0.5, (A.y + D.y) * 0.5};
    }

    const Vertex eab = IntersectEStarWithLine(a, b, c, A, B);
    const Vertex ecd = IntersectEStarWithLine(a, b, c, C, D);
    const double dab = ComputeProject2ArealDisplacement(A, B, C, D, eab);
    const double dcd = ComputeProject2ArealDisplacement(A, B, C, D, ecd);
    return dab <= dcd ? eab : ecd;
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
