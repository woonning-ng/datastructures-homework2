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
    if (div < EPSILON) return std::nullopt; // lines do not intersect (should be div == 0, because double precision)

    // otherwise, continue calculations and get intersection point
    Vertex d = { (float)Determinant(A), (float)Determinant(B) };

    Vertex result = { (float)Determinant(d, xdiff) / (float)div, (float)Determinant(d, ydiff) / (float)div };
    
    return result;
}

// should only be used in ComputePointE
bool DoLinesIntersect(Line A, Line B) {
    if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) return false;

    return true;
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
bool SegmentsIntersect(Line A, Line B) {

    return true;
}

bool IsCollinear(Vertex A, Vertex B, Vertex C) {

    return true;
}

// edge case in intersection checks
bool PointOnSegment(Vertex P, Line seg) {

    return true;
}

// orientation check
// returns -1, 0, 1 for clockwise, collinear, counterclockwise
// can be used in winding checks and intersection logic
int Orientation(Vertex A, Vertex B, Vertex C) {

    return 1;
}

// ASPC
Vertex ComputePointE(Line A, Line B) {
    
    return Vertex{};
}

double ArealDisplacement(Vertex A, Vertex B, Vertex C, Vertex D, Vertex E) {

    return 0.0;
}