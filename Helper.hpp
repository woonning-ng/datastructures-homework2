#ifndef HELPER_HPP
#define HELPER_HPP

#include "Polygon.hpp"

#include <map>
#include <string>
#include <vector>

int Read_CSV(const std::string& file_path, std::map<int, std::vector<Vertex>>& poly);
void Print_Poly(const std::string& file_path, const std::map<int, std::vector<Vertex>>& poly);

#endif
