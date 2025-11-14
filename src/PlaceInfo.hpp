#ifndef _PLACEINFO_HPP
#define _PLACEINFO_HPP

#include <mockturtle/mockturtle.hpp>
#include <cctype>
// #include "Hashtable.hpp"
using json = nlohmann::json;


namespace PlaceInfo {
    
    // int a(){
    //     return 10;
    // };
    int a();
    typedef struct {
        std::string name;
        std::string cell_type;
        bool isFixed;
        bool isPlaced;
        int orient;
        int x;
        int y;
    } Gate;

    typedef struct{
        std::string name;
        std::string type;
        std::string macroname;
        std::string cellname;
        int x;
        int y;
    } Pin;

    typedef struct{
        std::string name;
        // std::unordered_map<std::string, Pin> pins;
        std::vector<Pin> pins;
    } Net;

    typedef struct{
        std::string macroname;
        float area; 
        float height;
        float width;
    } Macro;

    template<typename T>
    using HashTable = std::unordered_map<std::string , T>;
    
    void construct_placeInfo_map(json def_data,HashTable<Gate>* PlaceInfoMap);
    void process_net(nlohmann::json net_data, HashTable<Net>* PlaceInfo_net, 
                    HashTable<Pin>* PlaceInfo_pin, HashTable<std::string>* PlaceInfo_gateToNet,HashTable<Gate>& PlaceInfoMap);
    void construct_gate_height_assignment_map(json gate_height_assignment_data,HashTable<std::string>* gateHeightAssignmentMap);
    void parseVerilogFile(const std::string& verilog_data, HashTable<std::string>& PlaceInfo_verilog);
    void rewrite_net_json(std::string design_name, const HashTable<Net>& PlaceInfo_net, const json& original_json); 
    void rewrite_def_json(std::string file_path, const HashTable<Gate>& PlaceInfoMap);
    void process_lef(nlohmann::json lef_data,HashTable<Macro>* PlaceInfo_macro);
}

#endif
