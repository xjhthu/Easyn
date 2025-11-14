#ifndef DP_H
#define DP_H
#include <mockturtle/mockturtle.hpp>
#include "resynthesis.hpp"
#include "Hashtable.hpp"
// #define MAXFLOAT 9999999
#define REMAP_FLAG 0

namespace DPSolver {
    struct DP_Gate_Sol{
        std::string name;
        std::string cell_type;
        // float delay;
        std::vector<float> delay_list;
        std::vector<float> area_list;
        float area;
        bool is_calculated;
        bool is_pruned;
        kitty::dynamic_truth_table gate_function;
        uint32_t gate_id;
        std::vector<uint8_t> permutation;
        std::vector<pin> pins;
        std::string output_name;
        mockturtle::cut<16, mockturtle::cut_data<true>>* cut;
        uint32_t vote; //counts of being selected as the dp solution
        std::vector<std::vector<std::string>> pareto_gate_list;
        std::vector<std::vector<std::string>> pareto_fanin_index_list; 
        // std::vector<std::string> best_gate_type_list; //The best combination of gate types selected for the cut
    } ;
    struct DP_Gate_Sol_Cache{
        std::vector<std::vector<std::string>> pareto_gate_list;
        std::vector<std::vector<std::string>> pareto_fanin_index_list;
        std::vector<uint32_t> max_fanin_list;
        std::vector<std::string> max_fanin_gate_list;
        std::vector<int> max_index_list;
    };
    template<typename T>
    using HashTable = std::unordered_map<std::string , T>;
    extern HashTable<int> dp_iter_map;
    extern HashTable<HashTable<DP_Gate_Sol>> dp_table;
    extern HashTable<HashTable<DP_Gate_Sol_Cache>> dp_cut_cache;
    extern HashTable<std::vector<DP_Gate_Sol>> dp_final_solution;
    extern HashTable<std::vector<uint32_t>> node_cut_map;
    extern HashTable<gate> node_to_gate_map;
    extern HashTable<std::string> repower_gate_name;
    extern HashTable<float> node_to_slack_map;
    extern std::vector<std::pair<std::string, float>> sorted_node_to_slack_map;
    extern float positive_slack_threshold;
    extern float negative_slack_threshold;
    extern HashTable<float> output_pin_delay_map;
    
    // void init_dp_table(binding_view<klut_network>* res,const std::vector<gate>& lib_gates, std::vector<gate>& gates, binding_cut_type& cuts);
    void dp_solve(binding_view<klut_network>* res,const std::vector<gate>& lib_gates,std::vector<gate>& gates,
                    binding_cut_type& cuts, HashTable<std::string> &gateHeightAssignmentMap, std::string design_name,
                    PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                    PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog,bool remap_flag);
    void dp_back_propagation();
    void naive_solution_selection(binding_view<klut_network>* res, std::string design_name,bool remap_flag,PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap);
    bool check_prune_in_dp_table_init(std::string n,std::string assigned_gate_height,std::string candidate_gate_name);
    void calculate_prune_threshold_in_dp_table_init();
    bool check_gate_height_match(std::string assigned_height, std::string gate_name);
    bool check_multi_stength_cell(std::string cell_name);
}


#endif