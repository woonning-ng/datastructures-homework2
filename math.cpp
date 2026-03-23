// File containing the definitions of various helper math functions
// that will be used in computations

#include "Math.hpp"

// LINKS
// https://www.geeksforgeeks.org/cpp/line-intersection-in-cpp/

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
// std::optional<Vertex> LineIntersection(Line A, Line B) {
//     // guards in case the vertices are null for whatever
//     // reason
//     if (A.a == nullptr || A.b == nullptr || B.a == nullptr || B.b == nullptr) return std::nullopt;


// }

// // should only be used in ComputePointE
// bool DoLinesIntersect(Line A, Line B) {

// }


// // -- POLYGON RELATED OPERATIONS -- 
// // signed area - shoelace method, sign tells orientation
// double SignedArea(std::vector<Vertex> polygon) {

// }

// double TriangleArea(Vertex A, Vertex B, Vertex C) {

// }

// // topology checks
// // should be used everywhere else intopology checks
// bool SegmentsIntersect(Line A, Line B) {

// }

// bool IsCollinear(Vertex A, Vertex B, Vertex C) {

// }

// // edge case in intersection checks
// bool PointOnSegment(Vertex P, Line seg) {

// }

// // orientation check
// // returns -1, 0, 1 for clockwise, collinear, counterclockwise
// // can be used in winding checks and intersection logic
// int Orientation(Vertex A, Vertex B, Vertex C) {

// }

// // ASPC
// Vertex ComputePointE(Line A, Line B) {
    
// }

// double ArealDisplacement(Vertex A, Vertex B, Vertex C, Vertex D, Vertex E) {

// }