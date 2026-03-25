// File containing the definitions of various helper math functions
// that will be used in computations

#include "Math.hpp"

// LINKS
// https://www.geeksforgeeks.org/cpp/line-intersection-in-cpp/
// https://stackoverflow.com/questions/20677795/how-do-i-compute-the-intersection-point-of-two-lines
// https://www.101computing.net/the-shoelace-algorithm/

// -- HELPERS --
double Determinant(Vertex A, Vertex B) {
    return A.x * B.y - A.y * B.x;
}

double Determinant(Line line) {
    return line.a->x * line.b->y - line.a->y * line.b->x;
}

// -- LINE RELATED OPERATIONS -- 

// dot product
// A.x * B.x + A.y * B.y
double DotProduct(Vector A, Vector B) {
    return A.x * B.x + A.y * B.y;
}

// cross product
// x = A.y * 0   - 0 * B.y = 0
// y = 0 * B.x   - A.x * 0 = 0
// z = A.x * B.y - A.y * B.x  -> this is the only surviving term
double CrossProduct(Vector A, Vector B) {
    return A.x * B.y - A.y * B.x;
}

// line intersection - should not return anything proper
// if lines are parallel. therefore it is encased in an optional
 std::optional<Vertex> LineIntersection(Line A, Line B) {
     // guards in case the vertices are null for whatever
     // reason
     if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) return std::nullopt;

    // xdiff = (line1[0][0] - line1[1][0], line2[0][0] - line2[1][0])
    // ydiff = (line1[0][1] - line1[1][1], line2[0][1] - line2[1][1])
    Vertex xdiff = { A.a->x - A.b->x, B.a->x - B.b->x };
    Vertex ydiff = { A.a->y - A.b->y, B.a->y - B.b->y };

    double div = Determinant(xdiff, ydiff);
    if (std::abs(div) < EPSILON) return std::nullopt; // lines do not intersect (should be div == 0, because double precision)

    // otherwise, continue calculations and get intersection point
    Vertex d = { Determinant(A), Determinant(B) };

    Vertex result = { Determinant(d, xdiff) / div, Determinant(d, ydiff) / div };
    
    return result;
}

// -- POLYGON RELATED OPERATIONS -- 
// signed area - shoelace method, sign tells orientation
// IMPORTANT: VERTICES MUST BE ENTERED IN ANTICLOCKWISE ORDER
double SignedArea(std::vector<Vertex> poly, bool reverse) {
    int noVertices = (int)polygon.size();
    double sum1; double sum2;

    // if polygon is not reversed already, reverse it
    std::vector<Vertex> polygon = poly;
    if (reverse) std::reverse(polygon.begin(), polygon.end());

    // tally all pairs minus the last
    for (int i = 0; i < noVertices - 1; i++) {
        sum1 += polygon[i].x * polygon[i + 1].y;
        sum2 += polygon[i].y * polygon[i + 1].x;
    }

    // add the last pair
    sum1 += polygon[noVertices - 1].x * polygon[0].y;
    sum2 += polygon[0].x * polygon[noVertices - 1].y;

    // find area
    return (sum1 - sum2) / 2.0;
}

// Area = | AB x AC | / 2
double TriangleArea(Vertex A, Vertex B, Vertex C) {
    return CrossProduct(B - A, C - A) / 2.0;
}

// topology checks
// should be used everywhere else intopology checks
// DoLinesIntersect  -> only fails if lines are PARALLEL (direction vectors same)
// SegmentsIntersect -> fails if parallel OR if intersection point is outside both segments
// lines - infinite line, segment - has end points
bool SegmentsIntersect(Line A, Line B) {
    if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) return false;

    int d1 = Orientation(*B.a, *B.b, *A.a);
    int d2 = Orientation(*B.a, *B.b, *A.b);
    int d3 = Orientation(*A.a, *A.b, *B.a);
    int d4 = Orientation(*A.a, *A.b, *B.b);

    // general case
    if (d1 != d2 && d3 != d4) return true;

    // collinear edge cases
    if (d1 == 0 && PointOnSegment(*A.a, B)) return true;
    if (d2 == 0 && PointOnSegment(*A.b, B)) return true;
    if (d3 == 0 && PointOnSegment(*B.a, A)) return true;
    if (d4 == 0 && PointOnSegment(*B.b, A)) return true;

    return false;
}

bool IsCollinear(Vertex A, Vertex B, Vertex C) {
    return Orientation(A, B, C) == 0;
}

// edge case in intersection checks
bool PointOnSegment(Vertex P, Line seg) {
    // P must be collinear with segment AND within the bounding box
    if (!IsCollinear(*seg.a, P, *seg.b)) return false;

    return std::min(seg.a->x, seg.b->x) <= P.x && P.x <= std::max(seg.a->x, seg.b->x) &&
    std::min(seg.a->y, seg.b->y) <= P.y && P.y <= std::max(seg.a->y, seg.b->y);
}

// orientation check
// returns -1, 0, 1 for clockwise, collinear, counterclockwise
// can be used in winding checks and intersection logic
int Orientation(Vertex A, Vertex B, Vertex C) {
    double cross = CrossProduct(
        {B.x - A.x, B.y - A.y},
        {C.x - A.x, C.y - A.y}
    );
    if (std::abs(cross) < EPSILON) return 0;  // collinear
    return (cross > 0) ? 1 : -1;              // 1=CCW, -1=CW
}

// ASPC
// Refer to the Kronenfield paper for this
Vertex ComputePointE(Vertex A, Vertex B, Vertex C, Vertex D) {
    
    return Vertex{};
}

double ArealDisplacement(Vertex A, Vertex B, Vertex C, Vertex D, Vertex E) {
     // areal displacement = area of symmetric difference between
    // original path A->B->C->D and new path A->E->D
    // = area of triangle ABE + area of triangle BCE + area of triangle CDE
    // but signed, so we take absolute values
    double t1 = TriangleArea(A, B, E);
    double t2 = TriangleArea(B, C, E);
    double t3 = TriangleArea(C, D, E);
    return std::abs(t1) + std::abs(t2) + std::abs(t3);
}