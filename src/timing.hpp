#ifndef TIMING_HPP
#define TIMING_HPP

#include <flute.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <tuple>
#include <cmath>
#include <map>
#include <ot/timer/timer.hpp>
#include "PlaceInfo.hpp"

#include <typeinfo>
// #include <mutex>

// Similar to the hyperparameters
#define WIRE_RESISTANCE_PER_MICRON 1.35
#define WIRE_CAPACITANCE_PER_MICRON 0.44e-15
#define UNIT_TO_MICRON 2000   


template<typename T>
using HashTable = std::unordered_map<std::string , T>;

struct hash_tuple{               // hash binary tuple
    template <class T>
    std::size_t operator()(const std::tuple<T, T>& tup) const {
        auto hash1 = std::hash<T>{}(std::get<0>(tup));
        auto hash2 = std::hash<T>{}(std::get<1>(tup));
        return hash1 ^ hash2;
    }
};

struct FluteResult{
    float tns;
    float wns;
};

struct Origin_info{
    std::vector<uint8_t> permutation;
    std::vector<mockturtle::pin> lib_pins;
    std::string output_name;
};

class SNode{
public:
    int x;                         // relative x coordinate
    int y;                         // relative y coordinate
    int idx;                       // Steiner nodes index 
    int pinIdxs;      // pins' indexes within the node
    std::set<int> toConnect;       // nodes connecting to this one

    int degree() {return toConnect.size();}
    SNode() : x(-1), y(-1), pinIdxs(-1) {}
    SNode(int nx, int ny) : x(nx), y(ny), 
                            pinIdxs(-1) {}
    SNode(std::tuple<int, int> loc) : x(std::get<0>(loc)), y(std::get<1>(loc)),
                                    pinIdxs(-1) {}
};


class createRCTree{
public:

    createRCTree(HashTable<PlaceInfo::Net>& PlaceInfo_net, HashTable<std::string>& PlaceInfo_gateToNet, 
                HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<std::string>& PlaceInfo_verilog)
                :PlaceInfo_net(PlaceInfo_net), PlaceInfo_gateToNet(PlaceInfo_gateToNet), PlaceInfoMap(PlaceInfoMap), 
                PlaceInfo_verilog(PlaceInfo_verilog),timer(nullptr)
                {
                    timer = new ot::Timer();
                }

    createRCTree(HashTable<PlaceInfo::Net>& PlaceInfo_net, HashTable<std::string>& PlaceInfo_gateToNet, HashTable<PlaceInfo::Gate>& PlaceInfoMap,
                HashTable<std::string>& PlaceInfo_verilog,ot::Timer& timer)
        :PlaceInfo_net(PlaceInfo_net), PlaceInfo_gateToNet(PlaceInfo_gateToNet), PlaceInfoMap(PlaceInfoMap), 
        PlaceInfo_verilog(PlaceInfo_verilog),timer(&timer) {}

    void buildTopo_global(std::string lib_file, std::string verilog_file, std::string sdc_file);

    void buildTopo_remap(std::vector<uint32_t>& original_cut, std::vector<uint32_t>& new_cut,
                             int node_index, std::string gate_name, std::vector<uint8_t> permutation, 
                             std::vector<mockturtle::pin> lib_pins, std::string output_name); 

    void buildTopo_test(std::string lib_file, std::string verilog_file, 
                        std::string spef_file, std::string sdc_file);
                         
    void report_timing();
    FluteResult runFlute();
    

private:

    float manhattanDistance(const std::tuple<int, int>& p1, const std::tuple<int, int>& p2);
    void print_snodes(const std::map<int, SNode>& snodes);
    
    std::vector<std::string> net_names; // Modified and reconstructed net name of rctree
    HashTable<PlaceInfo::Net>& PlaceInfo_net;
    HashTable<PlaceInfo::Gate>& PlaceInfoMap;
    HashTable<std::string>& PlaceInfo_gateToNet;
    HashTable<std::string>& PlaceInfo_verilog;
    ot::Timer* timer; // The timer containing RCTrees
    
};


#endif //TIMING_HPP