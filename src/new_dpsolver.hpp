#ifndef NEWDP_H
#define NEWDP_H
#include <mockturtle/mockturtle.hpp>
#include "resynthesis.hpp"
#include "Hashtable.hpp"
#include "../abacus/Legalizer/Legalizer.hpp"
#include "PlaceInfo.hpp"

namespace NewDPSolver {
    struct New_DP_Gate_Sol{
        std::string name;
        std::string cell_type;
        // float delay;
        float delay_list;
        // std::vector<float> area_list;
        double area;
        bool is_calculated;
        bool is_pruned;
        kitty::dynamic_truth_table gate_function;
        uint32_t gate_id;
        std::vector<uint8_t> permutation;
        std::vector<pin> pins;
        std::string output_name;
        mockturtle::cut<16, mockturtle::cut_data<true>>* cut;
        uint32_t vote; //counts of being selected as the dp solution
        // std::vector<std::string> pareto_gate_list;
        // std::vector<std::string> pareto_fanin_index_list; 
        // std::vector<std::string> best_gate_type_list; //The best combination of gate types selected for the cut
    } ;
    template<typename T>
    using HashTable = std::unordered_map<std::string , T>;
    extern HashTable<int> dp_iter_map;
    extern HashTable<std::vector<int>> dp_fanouts;
    extern HashTable<HashTable<New_DP_Gate_Sol>> dp_table;
    extern HashTable<HashTable<New_DP_Gate_Sol>> dp_cut_cache;
    extern HashTable<std::vector<New_DP_Gate_Sol>> dp_final_solution;
    extern HashTable<std::vector<uint32_t>> node_cut_map;
    extern HashTable<gate> node_to_gate_map;
    extern PlaceInfo::HashTable<PlaceInfo::Gate> initialPlaceInfoMap;
    extern PlaceInfo::HashTable<PlaceInfo::Macro> initialPlaceInfo_macro;
    extern HashTable<std::string> repower_gate_name;
    extern HashTable<float> node_to_slack_map;
    extern std::vector<std::pair<std::string, float>> sorted_node_to_slack_map;
    extern float positive_slack_threshold;
    extern float negative_slack_threshold;
    extern HashTable<float> output_pin_delay_map;
    extern double max_x_length;
    
    void New_dp_solve(binding_view<klut_network>* res,const std::vector<gate>& lib_gates,std::vector<gate>& gates,
                    binding_cut_type& cuts, HashTable<std::string> &gateHeightAssignmentMap, PlaceInfo::HashTable<PlaceInfo::Macro> PlaceInfo_macro, std::string design_name,
                    PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                    PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog,bool remap_flag, int chosen_top_K, int iter, double _area_slack, int _iteration_rowheight, int density_12T, int density_8T, double period_extra, double slack_rate_init=0.01);
}

#endif 