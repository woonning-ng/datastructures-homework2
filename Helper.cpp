#include "Helper.hpp"
#include <iostream>
#include <fstream>
#include <string>

int Read_CSV(std::string file_path, std::map<std::pair<std::string, std::string>, Vertex>& poly){
    std::ifstream ifs(file_path);

    if(!ifs.is_open()){
        std::cerr << "No such path exists\n";
        return 0;
    }
    std::string line{};
    //Skip first line
    std::getline(ifs, line);

    while(std::getline(ifs, line)){
        if(line.empty()){
            break;
        }
        std::size_t pos{};
        std::size_t cur_pos{};
        int var = 0;
        std::string ring_id{};
        std::string vertex_id{};

        Vertex v;
        while((pos = line.find(",", cur_pos)) != std::string::npos){
            switch(var){
                case 0:
                //Ring Id
                ring_id = line.substr(0, pos);
                //std::cout << "Ring Id: " << ring_id;
                break;

                case 1:
                vertex_id = line.substr(cur_pos, pos - cur_pos);
                //std::cout << " Vertex Id: " << vertex_id;
                break;

                case 2:
                v.x = std::stof(line.substr(cur_pos, pos - cur_pos));
                //std::cout << " X value: " << v.x;
                break;

            }
            var++;
            cur_pos = pos + 1;
        }
        v.y = std::stof(line.substr(cur_pos, line.size() - cur_pos));
        //std::cout << " Y Value: " << v.y << std::endl;
    }
    return 1;
}