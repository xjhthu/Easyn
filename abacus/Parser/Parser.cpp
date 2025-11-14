#include "Parser.hpp"
#include "Structure/Data.hpp"
#include <iostream>
#include <unordered_map>


void Parser::readAux(std::string const &filename)
{
    std::ifstream fin(filename);
    std::string buff;
    while (std::getline(fin, buff))
    {
        if (buff.empty())
            continue;

        std::stringstream buffStream(buff);
        std::string identifier, temp;
        buffStream >> identifier;
        if (identifier == "RowBasedPlacement")
        {
            buffStream >> temp >> this->nodeFile >> this->plFile >> this->sclFile;
        }
        else if (identifier == "MaxDisplacement")
        {
            buffStream >> temp >> this->maxDisplacement;
        }
    }
}

std::unordered_map<std::string, Cell *> strToCell;
json lef_data, def_data;
int row_width = 0;
std::unordered_map<int, float> row_density_map;
void Parser::readNode(std::string const &filename)
{
    std::ifstream fin(filename);
    size_t nodeNum = 0, terminalNum = 0;
    std::string buff;
    while (std::getline(fin, buff))
    {
        if (buff.empty())
            continue;

        std::stringstream buffStream(buff);
        std::string identifier, temp;
        buffStream >> identifier;
        if (identifier == "UCLA" || identifier.at(0) == '#')
        {
            continue;
        }
        else if (identifier == "NumNodes")
        {
            buffStream >> temp >> nodeNum;
        }
        else if (identifier == "NumTerminals")
        {
            buffStream >> temp >> terminalNum;
            break;
        }
    }
    std::getline(fin, buff);

    size_t const cellNum = nodeNum - terminalNum;
    for (size_t i = 0; i < nodeNum; ++i)
    {
        std::getline(fin, buff);
        std::stringstream buffStream(buff);
        std::string name;
        int width = 0, height = 0;
        buffStream >> name >> width >> height;
        auto cell = new Cell(name, width, height);
        if (i < cellNum)
            this->cells.emplace_back(cell);
        else
            this->terminals.emplace_back(cell);
        strToCell.emplace(name, cell);
    }
}

void Parser::readPl(std::string const &filename)
{
    std::ifstream fin(filename);
    std::string buff;
    std::getline(fin, buff);
    std::getline(fin, buff);

    for (size_t i = 0; i < this->cells.size() + this->terminals.size(); ++i)
    {
        std::getline(fin, buff);
        std::stringstream buffStream(buff);
        std::string name;
        double x = 0, y = 0;
        buffStream >> name >> x >> y;
        strToCell.at(name)->x = x;
        strToCell.at(name)->y = y;
    }
}

void Parser::readDef(std::string const &filename){
    std::ifstream fin(filename);
    size_t nodeNum = 0, terminalNum = 0;
    std::string buff;
    while (std::getline(fin, buff))
    {
        if (buff.empty())
            continue;
        int x = 0;
        int y=0;
        int siteNum = 0;
        int siteWidth = 0;
        std::stringstream buffStream(buff);
        std::string identifier, temp;
        buffStream >> identifier;
        if (identifier=="ROW"){
            buffStream >> temp >> temp >> x >> y >> temp >> temp >> siteNum >> temp >> temp >> temp >> siteWidth;
            // std::cout<<x<<" "<<y<<" "<<siteNum<<" "<<siteWidth<<"\n";
            auto row = new Row(siteWidth, 1024, y);
            if(row_width==0){
                row_width = siteNum * siteWidth;
            }
            row->subRows.emplace_back(new SubRow(x, x + siteWidth * siteNum));
            this->rows.emplace_back(row);
        }
       
        if(identifier=="COMPONENTS"){
            break;
        }
    }
     while (std::getline(fin, buff)){
        if (buff.empty())
            continue;
        std::stringstream buffStream(buff);
        std::string identifier, temp;
        buffStream >> identifier;
        // std::cout<<identifier<<"\n";

        if(identifier=="-"){
            std::string name, gate_type;
            int x,y;
            buffStream >> name >> temp >> temp >> temp >> temp;
            // std::cout<< name <<" "<<gate_type<<"\n";
            x = def_data[name]["x"];
            y = def_data[name]["y"];
            gate_type = def_data[name]["macroName"];
            auto cell = new Cell(name, lef_data[gate_type]["width"], lef_data[gate_type]["height"]);
            for(int i=0;i<this->rows.size();i++){
                // std::cout << y <<" "<< this->rows[i]->y<<"\n";
                if (y==this->rows[i]->y || y==(this->rows[i]->y+512)){
                    cell->row_index = i ;
                    break;
                }
                
            }
            row_density_map[cell->row_index]+= cell->width;
            cell->x = x;
            cell->y = y;
            if(gate_type.find("8T")!=std::string::npos){
                cell->gate_height = "8T";
               
            }
            else{
                cell->gate_height = "12T";
            }
            this->cells.emplace_back(cell);
        
            strToCell.emplace(name, cell);
        }
        if(identifier=="END"){
            break;
        }
     }
}
void Parser::readJson(std::string const &filename1, std::string const &filename2){
    std::ifstream f_net(filename1);
    lef_data = json::parse(f_net);
    std::ifstream f_def(filename2);
    def_data = json::parse(f_def);
}
void Parser::readScl(std::string const &filename)
{
    std::ifstream fin(filename);
    size_t rowNum = 0;
    std::string buff;
    while (std::getline(fin, buff))
    {
        if (buff.empty())
            continue;

        std::stringstream buffStream(buff);
        std::string identifier, temp;
        buffStream >> identifier;
        if (identifier == "UCLA" || identifier.at(0) == '#')
        {
            continue;
        }
        else if (identifier == "NumRows")
        {
            buffStream >> temp >> rowNum;
            break;
        }
    }

    for (size_t i = 0; i < rowNum; ++i)
    {
        int y = 0, height = 0, siteWidth = 0, x = 0, siteNum = 0;
        while (std::getline(fin, buff))
        {
            if (buff.empty())
                continue;

            std::stringstream buffStream(buff);
            std::string identifier, temp;
            buffStream >> identifier;
            if (identifier == "Coordinate")
            {
                buffStream >> temp >> y;
            }
            else if (identifier == "Height")
            {
                buffStream >> temp >> height;
            }
            else if (identifier == "Sitewidth")
            {
                buffStream >> temp >> siteWidth;
            }
            else if (identifier == "SubrowOrigin")
            {
                buffStream >> temp >> x >> temp >> temp >> siteNum;
            }
            else if (identifier == "End")
            {
                auto row = new Row(siteWidth, height, y);
                row->subRows.emplace_back(new SubRow(x, x + siteWidth * siteNum));
                this->rows.emplace_back(row);
                break;
            }
        }
    }
}

LegalizerInput *Parser::parse(std::string auxFilePath, std::string lefJson,std::string defJson,std::vector<int> reassign_minor_rows,std::vector<int> reassign_major_rows,std::pair<std::string, std::string> target)
{

    // readAux(auxFilePath);
    this->cells = {};
    this->rows = {};
    row_density_map = {};
    this->maxDisplacement = 0;
    // std::string inputPath = auxFilePath.substr(0, auxFilePath.find_last_of('/'));
    // readNode(inputPath + '/' + this->nodeFile);
    // readPl(inputPath + '/' + this->plFile);
    // readScl(inputPath + '/' + this->sclFile);
    readJson(lefJson,defJson);
    readDef(auxFilePath);
    // for (auto i : reassign_minor_rows){
    //     std::cout<<i<<"\n";
    // }
    sort(reassign_minor_rows.begin(),reassign_minor_rows.end());
    sort(reassign_major_rows.begin(),reassign_major_rows.end());

    for (int i=0;i<this->rows.size();i++){
        std::cout<< "row index "<<i<<" y "<<this->rows[i]->y<<" density "<<row_density_map[i]/row_width<<"\n";
        if(std::find(reassign_minor_rows.begin(),reassign_minor_rows.end(),this->rows[i]->y)!=reassign_minor_rows.end()
        || std::find(reassign_minor_rows.begin(),reassign_minor_rows.end(),this->rows[i]->y+512)!=reassign_minor_rows.end()){
            // std::cout<<target.first<<"\n";
            this->rows[i]->cell_height = target.first;
        }
        else{
            // std::cout<<target.second<<"\n";
            this->rows[i]->cell_height = target.second;
        }
    }


    return new LegalizerInput(this->maxDisplacement, this->cells, this->terminals, this->rows);
}
