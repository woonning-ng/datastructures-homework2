// Header file containing the declarations of various helper math functions
// that will be used in computations

#ifndef MATH_HPP
#define MATH_HPP

#include "Polygon.hpp" // will be performing operations on Line, Vertex

#include <optional>
#include <limits>
#include <algorithm>

#define EPSILON  std::numeric_limits<double>::epsilon()

// -- HELPERS --
double Determinant(Vertex A, Vertex B);
double Determinant(Line line);

// -- LINE RELATED OPERATIONS -- 

// dot product
double DotProduct(Vector A, Vector B);

// cross product
double CrossProduct(Vector A, Vector B);

// line intersection - should not return anything proper
// if lines are parallel. therefore it is encased in an optional
std::optional<Vertex> LineIntersection(Line A, Line B);


// -- POLYGON RELATED OPERATIONS -- 
// signed area - shoelace method, sign tells orientation
// IMPORTANT: VERTICES MUST BE ENTERED IN ANTICLOCKWISE ORDER
double SignedArea(std::vector<Vertex> poly, bool reverse = 0);

double TriangleArea(Vertex A, Vertex B, Vertex C);

// topology checks
// should be used everywhere else intopology checks
bool SegmentsIntersect(Line A, Line B);

bool IsCollinear(Vertex A, Vertex B, Vertex C);

// edge case in intersection checks
bool PointOnSegment(Vertex P, Line seg);  

// orientation check
// returns -1, 0, 1 for clockwise, collinear, counterclockwise
// can be used in winding checks and intersection logic
int Orientation(Vertex A, Vertex B, Vertex C);

// ASPC
Vertex ComputePointE(Vertex A, Vertex B, Vertex C, Vertex D);
double ArealDisplacement(Vertex A, Vertex B, Vertex C, Vertex D, Vertex E);

// distance functions
// for checking how close E lands to other rings
double DistancePointToSegment(Vertex P, Line seg);

double DistancePointToPoint(Vertex A, Vertex B);

// point containment - critical for hole checking
// ray casting algorithm
// checks if E lands inside a hole which would be topologically invalid
bool PointInRing(Vertex P, std::vector<Vertex> ring);

// winding number - related to above
// more robust than ray casting for edge cases
int WindingNumber(Vertex P, std::vector<Vertex> ring);

// just SignedArea < 0 basically
// but having it named explicitly makes the topology 
// validation code much more readable
bool IsRingClockwise(std::vector<Vertex> ring);


#endif