#pragma once
#include "../Structure/Data.hpp"
#include "../Structure/json.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using json = nlohmann::json;
extern json def_data;
extern int row_width;
extern std::unordered_map<int, float> row_density_map;
class Parser
{
    std::string nodeFile, plFile, sclFile;
    int maxDisplacement;
    std::vector<Cell *> cells, terminals;
    std::vector<Row *> rows;
    
    void readAux(std::string const &filename);
    void readNode(std::string const &filename);
    void readPl(std::string const &filename);
    void readScl(std::string const &filename);
    void readDef(std::string const &filename);
    void readJson(std::string const &filename1,std::string const &filename2);

public:
    Parser() : maxDisplacement(0) {}
    LegalizerInput *parse( std::string auxFilePath, std::string lefJson,std::string defJson,std::vector<int> reassign_minor_rows,std::vector<int> reassign_major_rows,std::pair<std::string, std::string> targe);
};
