//Header file for helper functions such as reading of csv
#include "Polygon.hpp"

//Return 1 if success and 0 if got error
int Read_CSV(std::string file_path, std::map<std::pair<std::string, std::string>, Vertex>& poly);
