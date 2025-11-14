#include "dp_solver.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <iostream>
#include <mockturtle/utils/tech_library.hpp>
#include "ot/headerdef.hpp"
#include "timing.hpp"
#include "util.hpp"
#include "omp.h"
using namespace DPSolver;
using namespace std;
using namespace Util;
namespace DPSolver {
    DPSolver::HashTable<DPSolver::HashTable<DP_Gate_Sol>> dp_table;
    DPSolver::HashTable<HashTable<DP_Gate_Sol_Cache>> dp_cut_cache;
    DPSolver::HashTable<std::vector<DP_Gate_Sol>> dp_final_solution;
    HashTable<int> dp_iter_map;
    HashTable<vector<uint32_t>> node_cut_map;
    HashTable<gate> node_to_gate_map;
    HashTable<string> repower_gate_name;
    HashTable<float> node_to_slack_map;
    HashTable<vector<uint32_t>> node_parent_map;
    std::vector<std::pair<string, float>> sorted_node_to_slack_map;
    HashTable<float> output_pin_delay_map;
    float positive_slack_threshold = 0.0f;
    float negative_slack_threshold = 0.0f;
    bool check_input_gate(binding_view<klut_network>* res, int32_t n){
        bool input_gate_flag = 1;
        res->foreach_fanin(n,[&](auto fanin){
            // std::cout<<fanin<<" is ci "<<res->is_ci(fanin)<<" is constant "<<res->is_constant(fanin)<<" result "<<(!res->is_ci(fanin) && !res->is_constant(fanin))<<"\n";
            if (!res->is_ci(fanin) && !res->is_constant(fanin)){
                // std::cout<<"am I here "<<n<<"\n";
                input_gate_flag = 0;
            }
        });
        return input_gate_flag;
    }
    //Inside the function init_dp_table, determine whether to maintain the original gate choice of node n or not;
    //Decision criteria: maintain the choise of nodes with positive slack and the same height as RHA.
    void calculate_prune_threshold_in_dp_table_init(){
        float positive_slack_ratio = 0.5; //the ratio of maintaining the original gate
        int positive_slack_cnt = 0;
        for(auto pair : sorted_node_to_slack_map){
            // std::cout<<pair.first<<" "<<pair.second<<"\n";
            if(pair.second>0) positive_slack_cnt += 1;
        }
        std::cout<<(positive_slack_cnt*1.0f/sorted_node_to_slack_map.size())<<"\n";
        if(positive_slack_cnt*1.0f/sorted_node_to_slack_map.size()>positive_slack_ratio){
            int index = (sorted_node_to_slack_map.size()*positive_slack_ratio);
            positive_slack_threshold = sorted_node_to_slack_map[index].second;
        }
        else{
            positive_slack_threshold = 0.0f;
        }
        
        negative_slack_threshold = sorted_node_to_slack_map[floor(0.9*sorted_node_to_slack_map.size())].second;
        // std::cout<<(int)0.8*sorted_node_to_slack_map.size()<<" "<<floor(sorted_node_to_slack_map.size()*0.8)<<"\n";
        std::cout<<"positive_slack_threshold "<<positive_slack_threshold<<"\n";
        std::cout<<"negative_slack_threshold "<<negative_slack_threshold<<"\n";
       

    }
    bool check_prune_in_dp_table_init(string n, string assigned_gate_height, string candidate_gate_name){
       
        if(check_gate_height_match(assigned_gate_height,node_to_gate_map[n].name)){
        // if(node_to_slack_map[n]>positive_slack_threshold && check_gate_height_match(assigned_gate_height,node_to_gate_map[n].name)){
            // std::cout<<"original name "<<node_to_gate_map[n].name<<" candidate name "<<candidate_gate_name<<"\n";
            if(node_to_gate_map[n].name==candidate_gate_name){
                // std::cout<<"gate maintained for "<<n<<": "<<candidate_gate_name<<"\n";
                return false;
            }
            return true;
        }
        return false;
    }

    bool check_multi_stength_cell(string cell_name){
        // return Util::find_substring_from_string("AND",cell_name) || Util::find_substring_from_string("AOI",cell_name) || 
        //         Util::find_substring_from_string("INV",cell_name) || Util::find_substring_from_string("BUF",cell_name) ||
        //         Util::find_substring_from_string("NAND",cell_name) ||
        //         Util::find_substring_from_string("OAI",cell_name);
        return Util::find_substring_from_string("AND",cell_name);
    } 
    bool check_gate_height_match(std::string assigned_height, std::string gate_name){
        if(assigned_height=="8T"){
            if(Util::find_substring_from_string("8T", gate_name)){
                return true;
            }
        }
        else if(assigned_height=="12T"){
            if(!Util::find_substring_from_string("8T", gate_name)){
                return true;
            }
        }
        return false;
    }
    void init_dp_table(binding_view<klut_network>* res,const std::vector<gate>& lib_gates,std::vector<gate>& gates,binding_cut_type& cuts, HashTable<std::string> &gateHeightAssignmentMap, ot::Timer &timer, bool remap_flag, PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap){
        tech_library tech_lib( gates );
        //construct node_to_slack_map
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            if (!res->is_ci(n) && !res->is_constant(n)){
                auto original_gate = lib_gates[res->get_binding_index( n )];
                float slack = *timer.report_slack("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
                // std::cout<<"slack of "<<n<<"="<<slack<<"\n";
                node_to_slack_map[std::to_string(n)] = slack;
            }
        });
        
        for (auto& i : node_to_slack_map)
        sorted_node_to_slack_map.push_back(i);

        std::sort(sorted_node_to_slack_map.begin(), sorted_node_to_slack_map.end(), 
          [=](std::pair<string, float>& a, std::pair<string, float>& b) { return a.second > b.second; });
        int positive_slack_cnt = 0;
        for(auto pair : sorted_node_to_slack_map){
            // std::cout<<pair.first<<" "<<pair.second<<"\n";
            if(pair.second>0) positive_slack_cnt += 1;
        }
        // std::cout<<"total nodes "<<sorted_node_to_slack_map.size()<<" positive slack num "<<positive_slack_cnt<<"\n";
        calculate_prune_threshold_in_dp_table_init();
        // exit(1);
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            // std::cout<<"node "<<n<<"\n";
            // std::cout<<"placeinfomap size begin"<<PlaceInfoMap.size()<<"\n";
            DPSolver::HashTable<DP_Gate_Sol> gate_sol_map;
            dp_table[std::to_string(n)] = gate_sol_map;
            dp_cut_cache[std::to_string(n)] = {};
            
            if (res->is_ci(n) || res->is_constant(n)){
                // std::cout<<"ci or constant "<<n<<"\n";
                DP_Gate_Sol gate_sol;
                gate_sol.name = std::to_string(n);
                gate_sol.cell_type = "pi";
                gate_sol.area = 0;
                gate_sol.delay_list = {0};
                gate_sol.vote = 0;
                gate_sol.is_calculated = true;
                gate_sol.is_pruned = false;
                
                dp_table[std::to_string(n)][std::to_string(n)] = gate_sol;
                // std::cout<<"ci or constant "<<n<<"\n";
            }
            
            // else if(check_input_gate(res, n)){ // for the gate whose fanins are all primary inputs, initialize the dp table
                // std::cout<<"topo order "<<n<<"is pi "<<res->is_ci(n)<<" is constant "<<res->is_constant(n)<<"\n";
               
            else{
               
            
                auto tt = res->node_function(n);
                kitty::static_truth_table<4> fe;
                if (tt.num_vars() < 4)
                    fe = kitty::extend_to<4>( tt );
                else
                    fe = kitty::shrink_to<4>( tt );  // control on four inputs
                auto supergates_pos = tech_lib.get_supergates( fe );
                auto index = res->node_to_index( n );
                
                auto original_gate = lib_gates[res->get_binding_index( n )];
                
                vector<uint32_t> original_gate_cut_list;
               
                res->foreach_fanin(n,[&](auto fanin){
                    original_gate_cut_list.push_back(fanin);
                });
                sort(original_gate_cut_list.begin(),original_gate_cut_list.end());
                node_cut_map[to_string(n)] = original_gate_cut_list;

                node_to_gate_map[to_string(n)] = original_gate;
                repower_gate_name[to_string(n)] = original_gate.name;
                //  std::cout<<"====================node"<<n<<"==================\n";
                // //  std::cout<< PlaceInfoMap[to_string(n)].name<<"\n";
                //  for (auto l : original_gate_cut_list){
                //     std::cout<<l<<" ";
                //  }
                //  std::cout<<"\n";
                float current_cut_distance = 0;
                float current_node_x = 0;
                float current_node_y = 0;
                // std::cout<<"placeinfomap size after cut distance before"<<PlaceInfoMap.size()<<"\n";
                // std::cout<<PlaceInfoMap.find("g"+to_string(n))->first<<"\n";
                // if ((PlaceInfoMap).find("g"+to_string(n))==(PlaceInfoMap).end()){
                //     std::cout<<"node g"<<n<<"not found!\n";
                //     current_cut_distance = -1;
                // }
                // else{
                    current_node_x = PlaceInfoMap["g"+to_string(n)].x;
                    current_node_y = PlaceInfoMap["g"+to_string(n)].y;
                    for (auto l : original_gate_cut_list){
                        int x,y;
                        if (res->is_ci(l)){
                            x = 0;
                            y = 0;
                        }
                        else{
                            x = PlaceInfoMap["g"+to_string(l)].x;
                            y = PlaceInfoMap["g"+to_string(l)].y;
                        }
                        // auto x = PlaceInfoMap["g"+to_string(l)].x;
                        // auto y = PlaceInfoMap["g"+to_string(l)].y;
                        current_cut_distance += Util::return_absolute_value(x-current_node_x);
                        current_cut_distance += Util::return_absolute_value(y-current_node_y);
                    }
                    current_cut_distance /= original_gate_cut_list.size();
                    // std::cout<<"placeinfomap suck after"<<PlaceInfoMap.size()<<"\n";
                // }
                
                // std::cout<<"current cut distance for "<<n<<" "<<current_cut_distance<<"\n";
                // for (auto l : original_gate_cut_list){

                // }
              
                auto & node_cuts = (cuts).cuts( index );
                char current_gate_name[100]; //tmp variable for permutation repeat
                auto gate_height = gateHeightAssignmentMap[to_string(n)];
               
                for ( auto& cut : node_cuts ){
                    strcpy(current_gate_name, "none");
                    
                    if ((cut->size()&&*cut->begin() == index) ==1 || cut->size()>4){
                        continue;
                    }
                    // std::cout<<" cut :"<<*cut<<'\n';
                    // if (check_input_gate(res, n)){
                    //     std::cout<<"node "<<n<<" cut :"<<*cut<<'\n';
                    //     for (auto pin : gate.root.pins){
                    //         std::cout<<pin.name<<"\n";
                    //     }
                    // }
                    
                    if (!remap_flag){ //if remap is not supported, keep only the original cut
                        vector<uint32_t> cut_list;
                        for(auto l : *cut){
                            cut_list.push_back(l);
                        }
                        if (original_gate_cut_list!=cut_list){
                            continue;
                        }
                    }
                    else{ // filter bad cut
                        
                        
                        if (current_cut_distance >0){
                            float new_cut_distance = 0;
                            for (int l : *cut){
                                int x,y;
                                if (res->is_ci(l)){
                                    x = 0;
                                    y = 0;
                                }
                                else{
                                    x = PlaceInfoMap["g"+to_string(l)].x;
                                    y = PlaceInfoMap["g"+to_string(l)].y;
                                }
                                // auto x = PlaceInfoMap["g"+to_string(l)].x;
                                // auto y = PlaceInfoMap["g"+to_string(l)].y;
                                
                                new_cut_distance += Util::return_absolute_value(x-current_node_x);
                                new_cut_distance += Util::return_absolute_value(y-current_node_y);
                            }
                         
                            new_cut_distance /= (*cut).size();
                            if (new_cut_distance*2 <= current_cut_distance || new_cut_distance == current_cut_distance){
                                if (new_cut_distance*2 < current_cut_distance){
                                    std::cout<<*cut<<"\n";
                                 std::cout<<"cut size "<<(*cut).size()<<"current_cut_distance "<<current_cut_distance<<" new_cust distance "<<new_cut_distance<<"\n";
                                }
                                
                            }
                            else{
                                continue;
                            }
                           
                        }
                        
                    }
                    // if(n==352)cout<<"cao "<<PlaceInfoMap["g"+std::to_string(n)].cell_type<<endl;
                     

                    auto tt = (cuts).truth_table( *cut );
                    auto fe = kitty::shrink_to<4>( tt );
                    auto supergates_pos = tech_lib.get_supergates( fe );
                    
                    if (supergates_pos !=nullptr){
                         for ( auto &gate : *supergates_pos ){
                            

                            if (unsigned(gate.polarity)!=0){ // skip nodes which require negation
                                continue;
                            }  

                            if (strcmp(lib_gates[gate.root->id].name.c_str(),current_gate_name)==0){//skip same gate with different pin permutation
                                continue;
                            }

                            //filter gate with minor height
                            // std::cout<<"gate name "<<lib_gates[gate.root->id].name.c_str()<<"\n";
                            if(!check_gate_height_match(gate_height,lib_gates[gate.root->id].name )){
                                continue;
                            }
                            // if(gate_height=="8T"){
                            //     if(!Util::find_substring_from_string("8T", lib_gates[gate.root->id].name)){
                            //         continue;
                            //     }
                            // }
                            // else if(gate_height=="12T"){
                            //      if(Util::find_substring_from_string("8T", lib_gates[gate.root->id].name)){
                            //         continue;
                            //     }
                            // }

                            // Pass the CLK gate
                            if(Util::find_substring_from_string("CLK",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass the TBUF gate
                            if(Util::find_substring_from_string("TBUF",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 16X gate
                            if(Util::find_substring_from_string("_X16_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 12X gate
                            if(Util::find_substring_from_string("_X12_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 8X gate
                            if(Util::find_substring_from_string("_X8_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // Pass 4X gate
                            if(Util::find_substring_from_string("_X4_",lib_gates[gate.root->id].name)){
                                continue;
                            }

                            if(check_prune_in_dp_table_init(to_string(n),gate_height,lib_gates[gate.root->id].name)){
                                continue;
                            }

                            // if(gate_height=="12T" && Util::find_substring_from_string("X1",lib_gates[gate.root->id].name) && check_multi_stength_cell(lib_gates[gate.root->id].name)){
                            //     std::cout<<"skip "<<lib_gates[gate.root->id].name<<"\n";
                            //     continue;
                            // }

                            // if(node_to_slack_map[to_string(n)]<=positive_slack_threshold && Util::find_substring_from_string("X1",lib_gates[gate.root->id].name) && check_multi_stength_cell(lib_gates[gate.root->id].name)){
                            //     continue;
                            // }


                            
                            // std::cout<<"gate name "<<lib_gates[gate.root->id].name<<"output name"<<lib_gates[gate.root->id].output_name<<"\n";
                            // for (auto pin : lib_gates[gate.root->id].pins){
                            //     std::cout<<"pin "<<pin.name<<" ";
                                
                            // }
                            // std::cout<<"\n";
                            DP_Gate_Sol gate_sol;
                            // std::cout <<"check input gate:";
                            // std::cout<<check_input_gate(res, n)<<"\n";
                            // std::cout<<"gate name "<<lib_gates[gate.root->id].name<<" polarity "<<signed(gate.polarity)<<"\n";
                            // for (auto p : gate.permutation){
                            //     std::cout <<unsigned(p)<<" ";
                            // }
                            // std::cout<<"\n";
                            if (check_input_gate(res, n)){ //for input gates
                            auto ctr = 0;
                            // HashTable<string> pin_name_map;
                            // for (int i=0; i< lib_gates[gate.root->id].pins.size();i++){
                            //     pin_name_map[original_gate.pins[i].name] = lib_gates[gate.root->id].pins[i].name;
                            //     // std::cout<<original_gate.pins[i].name<<" "<<lib_gates[gate.root->id].pins[i].name<<"\n";
                            // }
                            // pin_name_map[original_gate.output_name] = lib_gates[gate.root->id].output_name;
                            timer.repower_gate("g"+to_string(n),lib_gates[gate.root->id].name);
                           
                            float tmp_delay = *timer.report_at("g"+to_string(n)+":"+original_gate.output_name, ot::MAX,ot::FALL);
                            // std::cout<<"node "<<n<<" gate name "<<lib_gates[gate.root->id].name<<" output name "<<lib_gates[gate.root->id].output_name<<" tmp delay "<<tmp_delay<<"\n";
                            gate_sol.name = lib_gates[gate.root->id].name;
                            gate_sol.cell_type = "input_gate";
                            gate_sol.area = lib_gates[gate.root->id].area; 
                            gate_sol.delay_list = {tmp_delay};
                            gate_sol.cut = cut;
                            gate_sol.vote = 0;
                            gate_sol.permutation = gate.permutation;
                            gate_sol.gate_function = gate.root->function;
                            gate_sol.gate_id = gate.root->id;
                            gate_sol.pins = lib_gates[gate.root->id].pins;
                            gate_sol.output_name = lib_gates[gate.root->id].output_name;
                            gate_sol.is_calculated = false;
                            gate_sol.is_pruned = false;
                            
                            }
                            else{// for normal gates not calculated by dp
                            gate_sol.name = lib_gates[gate.root->id].name;
                            gate_sol.cell_type = "normal_gate";
                            gate_sol.area = lib_gates[gate.root->id].area; 
                            gate_sol.delay_list = {}; 
                            gate_sol.cut = cut;
                            gate_sol.vote = 0;
                            gate_sol.permutation = gate.permutation;
                            gate_sol.gate_function = gate.root->function;
                            gate_sol.gate_id = gate.root->id;
                            gate_sol.pins = lib_gates[gate.root->id].pins;
                            gate_sol.output_name = lib_gates[gate.root->id].output_name;
                            gate_sol.is_calculated = false;
                            gate_sol.is_pruned = false;
                            }
                            
                            
                         dp_table[std::to_string(n)][lib_gates[gate.root->id].name] = gate_sol;
                        
                        //  current_gate_name = lib_gates[gate.root->id].name.c_str();
                        strcpy(current_gate_name, lib_gates[gate.root->id].name.c_str());
                        //  std::cout<<"placeinfomap size after cut distance after"<<PlaceInfoMap.size()<<"\n";
                         }
                    }
                }
                if (check_input_gate(res, n)){
                    timer.repower_gate("g"+to_string(n),original_gate.name);
                }
                
            }
            
                // std::cout<<"gate name "<< gate.name << "node index" << index <<std::endl;
        // std::cout<<"placeinfomap size end"<<PlaceInfoMap.size()<<"\n\n";
        } );
        int total_gate = 0;
        int single_gate = 0;
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            total_gate+=1;
            if(dp_table[std::to_string(n)].size()==1)
                single_gate +=1;
            if(dp_table[std::to_string(n)].size()>=2){
                timer.repower_gate("g"+to_string(n), dp_table[std::to_string(n)].begin()->second.name);
                repower_gate_name[to_string(n)] = dp_table[std::to_string(n)].begin()->second.name;
                std::cout<<"multiple candidate "<<dp_table[std::to_string(n)].size()<<"\n";
            }
        });
        std::cout<<"single "<<single_gate<<" total "<<total_gate<<"\n";
    }

    DP_Gate_Sol dp_solve_rec(binding_view<klut_network>* res, uint32_t n, std::string gate_name, mockturtle::tech_library<4, mockturtle::classification_type::np_configurations> tech_lib,
                                binding_cut_type& cuts,const std::vector<gate>& lib_gates,ot::Timer &timer,
                                PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                                PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog)
    {
        // std::cout<<"enter dp_solve_rec "<<n<<" "<<gate_name<<"\n"; 
        // If the dp entry is already calculated, return the result
        if(dp_table[std::to_string(n)][gate_name].is_calculated){
            // std::cout<<"gate "<<gate_name << " of "<<n<<" is calculated\n";
            return dp_table[std::to_string(n)][gate_name];
        }

        
        // auto tt = res->node_function(n);
        // kitty::static_truth_table<4> fe;
        // if (tt.num_vars() < 4)
        //     fe = kitty::extend_to<4>( tt );
        // else
        //     fe = kitty::shrink_to<4>( tt );  // control on four inputs
       
        auto index = res->node_to_index( n );
        // std::cout<<"n "<<n<<" index "<<index<<"\n";

        auto & node_cuts = (cuts).cuts( index );
        char current_gate_name[100]; //tmp variable for permutation repeat
        vector<vector<string> > allVecs; //2-d vectors, store all permutations of the gates slected for all fanins
        vector<vector<string>> pareto_all_indexes;
        vector<std::vector<string> > fanin_permutation_list;
        vector<uint32_t> cut_list;
        DPSolver::HashTable<float> pin2pin_delay_map; //pin2pin_delay_map[l][i]: the delay from gate i in fanin l to the fanout of current node n
        float optimal_area = MAXFLOAT;
        float optimal_delay = MAXFLOAT;
        vector<float> area_list;
        vector<float> delay_list;
        vector<uint32_t> max_fanin_list;
        vector<int> max_index_list;
        vector<string> max_fanin_gate_list;
        vector<vector<string>> fanin_perm_index_list;
        int cut_value = 0;
        
        // std::cout<<"current cut in dp for the node "<<n<<" "<<*(dp_table[std::to_string(n)][gate_name].cut)<<"\n";
        for (uint32_t l : *(dp_table[std::to_string(n)][gate_name].cut)){
            cut_list.push_back(l);
            cut_value += l;
            // std::cout<<l<<" ";
        }
        
        if (node_cut_map[to_string(n)]!=cut_list){
            //TODO: reconstruct rc tree in opentimer
            std::cout<<"n "<<n<<" look at the pin of gate "<<gate_name<<" output name "<<dp_table[to_string(n)][gate_name].output_name<<":\n";
            // std::cout<<lib_gates[dp_table[to_string(n)][gate_name].gate_id].name<<"\n";
            for (auto pin :dp_table[to_string(n)][gate_name].pins){
                std::cout<<pin.name<<" ";
            }
            // std::cout<<"\n";
            // for (auto p :dp_table[to_string(n)][gate_name].permutation){
            //     std::cout<<p<<" ";
            // }
            // std::cout<<"\n";
            
            createRCTree* tree = new createRCTree(PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap, PlaceInfo_verilog, timer);
             
            FluteResult result;
            tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n,  gate_name, dp_table[to_string(n)][gate_name].permutation, dp_table[to_string(n)][gate_name].pins, dp_table[to_string(n)][gate_name].output_name);
            result = tree->runFlute();
            delete tree;

            // update the global hashmap
            node_cut_map[to_string(n)] = cut_list;
            node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

            // tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n, gate_name, dp_table[to_string(n)][gate_name].permutation);
            std::cout<<"opentimer cut for "<<n<<":";
            for (auto l : node_cut_map[to_string(n)]){
                std::cout<<l<<" ";
            }
            std::cout<<"\n";
            std::cout<<"dp cut for "<<n<<":";
            for (auto l : cut_list){
                std::cout<<l<<" ";
            }
            std::cout<<"\n";
            timer.repower_gate("g"+to_string(n), gate_name);
        }
        // std::cout<<"repower!\n";
        if(dp_table[std::to_string(n)].size()>1){
            std::cout<<repower_gate_name[std::to_string(n)]<<" "<<gate_name<<"\n";
            if(repower_gate_name[std::to_string(n)]!=gate_name){
                // timer.repower_gate("g"+to_string(n), gate_name);
            //    std::cout<<"--------------repower required "<<n<<"\n";
            }
            else{
                // std::cout<<"----------------no need to repower "<<n<<"\n";
            }
            // std::cout<<"node "<<n<<" "<<gate_name<<" size "<<dp_table[std::to_string(n)].size()<<"\n";
            
        }
        // std::cout<<"\n";
        // std::cout<<"current cut "<<*(dp_table[std::to_string(n)][gate_name].cut)<<"\n";
        
        HashTable<float> faninArrivalTimeMap;
        auto start = std::chrono::high_resolution_clock::now(); 
        float current_gate_arrival_time = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // std::cout<<"at g"+to_string(n)<<' '<<current_gate_arrival_time<<"\n";
        // timer.debug("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // if(output_pin_delay_map.find("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name)==output_pin_delay_map.end()){
        //     output_pin_delay_map["g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name] = current_gate_arrival_time;
        // }
        // else{
        //     current_gate_arrival_time = *timer.report_at("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        // }
       
         auto end = std::chrono::high_resolution_clock::now();
    auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
    // std::cout<<"current gate report_at takes "<<time_span.count()<<" seconds !!!\n";
        for(auto fanin : *(dp_table[std::to_string(n)][gate_name].cut)){
            if (res->is_ci(fanin)||res->is_constant(fanin)){
                faninArrivalTimeMap[to_string(fanin)]=0.0;
               
            }
            else{
                std::string output_pin_name = "g"+to_string(fanin)+":"+node_to_gate_map[to_string(fanin)].output_name;
                 auto start = std::chrono::high_resolution_clock::now();
                faninArrivalTimeMap[to_string(fanin)] = *timer.report_at(output_pin_name,ot::MAX,ot::FALL);
                // cout<<"fanin g"+to_string(fanin)<<' '<<faninArrivalTimeMap[to_string(fanin)]<<'\n';
                  auto end = std::chrono::high_resolution_clock::now();
            auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
            // std::cout<<"repower takes "<<time_span.count()<<" seconds !!!\n";
                //  if(output_pin_delay_map.find(output_pin_name)!=output_pin_delay_map.end()){
                //      faninArrivalTimeMap[to_string(fanin)] = output_pin_delay_map[output_pin_name];
                //  }
                //  else{
                //     faninArrivalTimeMap[to_string(fanin)] = *timer.report_at(output_pin_name,ot::MAX,ot::FALL);
                //     output_pin_delay_map[output_pin_name] = faninArrivalTimeMap[to_string(fanin)];
                //  }
                
                // std::cout<<"delay of fanin "<<fanin<<" "<<faninArrivalTimeMap[to_string(fanin)]<<"\n";
            }
            
        }
        // std::cout<<"current_node "<<n<<" delay:"<<current_gate_arrival_time<<"\n";
        // int pin_ctr = 0;

        //Use Cache result to determine gate solution
        if(dp_cut_cache[std::to_string(n)].find(std::to_string(cut_value))!=dp_cut_cache[to_string(n)].end()){
            std::cout<<"node "<<n<<" cache exists "<<dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_list.size()<<" "<<dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_gate_list.size()<<" "<<dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_index_list.size()<<"\n";

            dp_table[std::to_string(n)][gate_name].is_calculated = true;
            dp_table[std::to_string(n)][gate_name].pareto_gate_list = dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].pareto_gate_list;
            dp_table[std::to_string(n)][gate_name].pareto_fanin_index_list = dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].pareto_fanin_index_list;

            for(int i=0;i<dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].pareto_gate_list.size();i++){
                
                uint32_t fanin = dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_list[i];
                string fanin_gate = dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_gate_list[i];
                int index = dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_index_list[i];
                // std::cout<<fanin<<" "<<fanin_gate<<" "<<index<<"\n";
                float fanin_delay = dp_table[to_string(fanin)][fanin_gate].delay_list[index];
                // std::cout<<fanin<<" "<<fanin_gate<<" "<<index<<" "<<fanin_delay<<"\n";
                float p2p_delay = current_gate_arrival_time - faninArrivalTimeMap[to_string(fanin)];
                // std::cout<<"p2p_delay "<<p2p_delay<<" fanin_delay "<<fanin_delay<<"\n";
                 dp_table[std::to_string(n)][gate_name].delay_list.push_back(fanin_delay+p2p_delay);
                
            }
            // exit(1);
            //     string gate_name = dp_table[std::to_string(n)][gate_name].pareto_gate_list[i];
            //     float fanin_delay = dp_table[std::to_string(cut_list[i])][gate_name].delay_list[dp_table[std::to_string(n)][gate_name].pareto_fanin_index_list[i]];
            //     float p2p_delay = current_gate_arrival_time - faninArrivalTimeMap[to_string(cut_list[i])];
            //     dp_table[std::to_string(n)][gate_name].delay_list.push_back(fanin_delay+p2p_delay);
            // }
            
        }
        else{
            // start = std::chrono::high_resolution_clock::now();
             
            for(auto fanin : *(dp_table[std::to_string(n)][gate_name].cut)){
                
                
                // std::cout<<"fanin "<<fanin<<"\n";
                vector<string> one_node_permutation;
                // float input_pin_delay = *timer.report_at("g"+to_string(n)+":"+dp_table[std::to_string(n)][gate_name].pins[pin_ctr].name ,ot::MAX,ot::FALL);
                // std::cout<<"pin "<<pin_ctr<<" o2o delay "<<current_gate_arrival_time - faninArrivalTimeMap[to_string(fanin)]<<" o2i delay "<<current_gate_arrival_time - input_pin_delay<<"\n";
                // pin_ctr+=1;
                pin2pin_delay_map[to_string(fanin)] = current_gate_arrival_time - faninArrivalTimeMap[to_string(fanin)];
                // std::cout<<"fanin gate num "<<dp_table[to_string(fanin)].size()<<"\n";
                // if(dp_table[to_string(fanin)].size()>2){
                //     dp_table_gate_pruning(dp_table[to_string(fanin)]);
                // }
              
                for(auto gate_sol:dp_table[to_string(fanin)]){
                    if(gate_sol.second.is_pruned) continue;
                    one_node_permutation.push_back(gate_sol.second.name);
                    // std::cout<<"fucking name "<<gate_sol.second.name<<"\n";
                    
                    
                    // pin2pin_delay_map[to_string(fanin)][gate_sol.second.name] = (float)rand()/(float)(RAND_MAX/10);
                    // std::cout <<"delay of "<<fanin<<" and "<<gate_sol.second.name<<": "<<pin2pin_delay_map[to_string(fanin)][gate_sol.second.name]<<"\n";
                        
                }
                allVecs.push_back(one_node_permutation);               
            }
            // exit(1);
            vector<std::vector<string> > permutation_list;
            printAll(allVecs, 0,"",permutation_list);
        
            // std::cout<<"total permutation list size "<<permutation_list.size()<<"\n";
            // for(auto l : permutation_list){
            //     for(auto g : l){
            //         std::cout<<g<<" ";
            //     }
            //     std::cout<<"\n";
            // }
            for(auto l : permutation_list){
                int current_best_index;
                std::vector<string> current_best_fanin_index;
                uint32_t current_max_fanin;
                std::string current_max_fanin_gate;
                int current_max_index;
                float current_max_delay = MAXFLOAT;
                float current_total_area = dp_table[std::to_string(n)][gate_name].area;
                // std::cout<<"l size "<<l.size()<<"\n";
                // #pragma omp parallel for default(shared)
                for(int i=0;i<l.size();i++){
                    // std::cout<<to_string(cut_list[i])<<" "<<l[i]<<"\n";
                auto current_dp_entry = dp_solve_rec(res, cut_list[i], l[i], tech_lib, cuts, lib_gates,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog);
                
                    // auto current_delay = current_dp_entry.delay + pin2pin_delay_map[to_string(cut_list[i])][l[i]];
                    
                    // if (current_delay >= current_max_delay){
                    //     current_max_delay = current_delay;
                        
                    // }
                    // #pragma omp critical
                    // {
                        current_total_area += current_dp_entry.area;
                    // }
                    // std::cout<<"after iter\n";
                }

                pareto_all_indexes = {};
                fanin_permutation_list = {};
    
                for (int i=0; i<l.size();i++){
                    // std::cout<<l[i]<<"\n";
                    vector<string> one_fanin_permutation;
                 
                    for (int j=0;j<dp_table[to_string(cut_list[i])][l[i]].delay_list.size();j++){
                        one_fanin_permutation.push_back(to_string(j));
                    }
                    // std::cout <<" size of current one_fanin_permutation "<<one_fanin_permutation.size()<<"\n";
                    pareto_all_indexes.push_back(one_fanin_permutation);
                }
                printAll(pareto_all_indexes, 0,"",fanin_permutation_list);
                // std::cout<<"node "<<n<<" show fanin permutation: "<<fanin_permutation_list.size()<<"\n";
                for (auto fanin_perm : fanin_permutation_list){
                    float local_max_delay=0;
                    
                    int local_max_index;
                    uint32_t local_max_fanin;
                    std::string local_max_fanin_gate;
                    // for(auto l : fanin_perm){
                    //     std::cout<<l<<" ";
                    // }
                    // std::cout<<"\n";
                    // std::cout<<"fanin_size "<<fanin_perm.size()<<"\n";
                    for (int i=0;i<fanin_perm.size();i++){
                    
                        auto current_delay = dp_table[to_string(cut_list[i])][l[i]].delay_list[stoi(fanin_perm[i])]+pin2pin_delay_map[to_string(cut_list[i])];
                        // std::cout<<l[i]<<" current_delay "<<current_delay<<" local max delay "<<local_max_delay<<"\n";
                        if (current_delay>local_max_delay){
                            local_max_delay  = current_delay;
                            local_max_fanin = cut_list[i];
                            local_max_fanin_gate = l[i];
                            local_max_index = stoi(fanin_perm[i]);
                            
                            
                        }
                    }
                    if (local_max_delay < current_max_delay){
                        // std::cout<<"find!\n";
                        current_max_delay = local_max_delay;
                        current_best_fanin_index = fanin_perm;
                        current_max_fanin = local_max_fanin;
                        current_max_index = local_max_index;
                        current_max_fanin_gate = local_max_fanin_gate;
                    }
                }
                // std::cout<<"before push\n";
                // std::cout<<"current max delay "<<current_max_delay<<" current_total_area "<<current_total_area<< "\n";
                delay_list.push_back(current_max_delay);
                area_list.push_back(current_total_area);
                max_fanin_list.push_back(current_max_fanin);
                max_index_list.push_back(current_max_index);
                max_fanin_gate_list.push_back(current_max_fanin_gate);
                fanin_perm_index_list.push_back(current_best_fanin_index);
                // std::cout<<"current best fanin index: ";
                // for(auto l : current_best_fanin_index){
                //     std::cout<<l<<" ";
                // }
                // std::cout<<"\n";
                // std::cout <<"before pareto i delay "<< current_max_delay<<"\n";
                
            }
            // end = std::chrono::high_resolution_clock::now();
            // time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
            // std::cout<<"One dp processing took: "<<time_span.count()<<" seconds !!!\n";
            auto best_index_list = pareto_solution_select_tmp(area_list, delay_list);
            dp_table[std::to_string(n)][gate_name].is_calculated = true;
            // std::cout<<"node "<<n<<" candidate list length "<<area_list.size()<<" best_index_list length "<<best_index_list.size()<<"\n";
            // std::cout<<"node "<<n<<" best permutation index :";
            // for(auto l : best_index_list){
            //     std::cout<<l<<" ";
            // }
            // std::cout<<"\n";
            vector<uint32_t> tmp_fanin_list;
            vector<string> tmp_fanin_gate_list;
            vector<int> tmp_index_list;
            for (auto i : best_index_list){
                // std::cout <<"i "<<i<<" delay "<< delay_list[i]<<"\n";
                dp_table[std::to_string(n)][gate_name].delay_list.push_back(delay_list[i]);
                dp_table[std::to_string(n)][gate_name].pareto_gate_list.push_back(permutation_list[i]);
                dp_table[std::to_string(n)][gate_name].pareto_fanin_index_list.push_back(fanin_perm_index_list[i]);
                tmp_fanin_list.push_back(max_fanin_list[i]);
                tmp_index_list.push_back(max_index_list[i]);
                tmp_fanin_gate_list.push_back(max_fanin_gate_list[i]);
                
                // for (auto l : fanin_perm_index_list[i]){
                //     std::cout<<l<<" ";
                // }
                // std::cout<<"\n";
                // for (auto g : permutation_list[i]){
                //     std::cout<<g<<" ";
                // }
                // std::cout<<"\n";
            }
            // std::cout<<"node "<<n<<" cut value "<<cut_value<<"\n";
            if(dp_cut_cache[std::to_string(n)].find(std::to_string(cut_value))==dp_cut_cache[to_string(n)].end()){
                DP_Gate_Sol_Cache gate_cut_cache;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)] = gate_cut_cache;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].pareto_gate_list = dp_table[std::to_string(n)][gate_name].pareto_gate_list;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].pareto_fanin_index_list = dp_table[std::to_string(n)][gate_name].pareto_fanin_index_list;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_list = tmp_fanin_list;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_fanin_gate_list = tmp_fanin_gate_list;
                dp_cut_cache[std::to_string(n)][std::to_string(cut_value)].max_index_list = tmp_index_list;
            }
        }
        
        // for(int i=0;i<cut_list.size();i++){
        //     dp_table[to_string(cut_list[i])][permutation_list[best_index][i]].vote+=1;
        // }
        // timer.repower_gate("g"+to_string(n), node_to_gate_map[to_string(n)].name);
        std::cout<<"gate "<<n<<" "<<gate_name<<" is calculated"<<' '<<dp_table[std::to_string(n)][gate_name].delay_list[0]<<"\n";
        
        return dp_table[std::to_string(n)][gate_name];
        
        
    }
    void dp_back_propagation_rec(binding_view<klut_network>* res, uint32_t n, uint32_t parent, string gate_name, HashTable<int> dp_iter_map,ot::Timer &timer,PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap,PlaceInfo::HashTable<std::string> &PlaceInfo_verilog,const std::vector<gate>& lib_gates,HashTable<std::string> &gateHeightAssignmentMap){
        if(std::find(node_parent_map[to_string(n)].begin(),node_parent_map[to_string(n)].end(),parent)==node_parent_map[to_string(n)].end()){
            node_parent_map[to_string(n)].push_back(parent);
        }
        else{
            return;
        }
        // std::cout<<"node "<<n<<" parent "<<parent<<"\n";
        // std::cout<<"inside back_pro "<<n<<" "<<gate_name<<" require time <<"<<*timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL)<<"\n";
        dp_iter_map[to_string(n)] +=1;
        if(dp_iter_map[to_string(n)]>3){
            return;
        }
        if(!find_DP_gate_sol_from_list(dp_table[to_string(n)][gate_name],dp_final_solution[to_string(n)])){
            dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)][gate_name]);
        }
        vector<uint32_t> cut_list;
        for (auto l : *(dp_table[to_string(n)][gate_name].cut)){
            cut_list.push_back(l);
        }
        // if (node_cut_map[to_string(n)]!=cut_list){
        //     //TODO: reconstruct rc tree in opentimer
        //     std::cout<<"n "<<n<<" look at the pin of gate "<<gate_name<<" output name "<<dp_table[to_string(n)][gate_name].output_name<<":\n";
        //     // std::cout<<lib_gates[dp_table[to_string(n)][gate_name].gate_id].name<<"\n";
        //     for (auto pin :dp_table[to_string(n)][gate_name].pins){
        //         std::cout<<pin.name<<" ";
        //     }
        //     // std::cout<<"\n";
        //     // for (auto p :dp_table[to_string(n)][gate_name].permutation){
        //     //     std::cout<<p<<" ";
        //     // }
        //     // std::cout<<"\n";
        //     createRCTree* tree = new createRCTree(PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap, PlaceInfo_verilog, timer);
        //     FluteResult result;
        //     tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n,  gate_name, dp_table[to_string(n)][gate_name].permutation, dp_table[to_string(n)][gate_name].pins, dp_table[to_string(n)][gate_name].output_name);
        //     result = tree->runFlute();
        //     delete tree;

        //     // update the global hashmap
        //     node_cut_map[to_string(n)] = cut_list;
        //     node_to_gate_map[to_string(n)] = lib_gates[dp_table[to_string(n)][gate_name].gate_id];

        //     // tree->buildTopo_remap(node_cut_map[to_string(n)], cut_list, n, gate_name, dp_table[to_string(n)][gate_name].permutation);
          
        // }

        // std::cout<<"hi "<<dp_table[to_string(n)][gate_name].delay_list[0]<<"\n";
        int selected_index = -1;
        if(dp_table[to_string(n)][gate_name].pareto_gate_list.size()==1){
            std::cout<<"size == 1";
            selected_index = 0;
        }
        else{
                float require_time = *timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        
        
            int min_index = return_min_value(dp_table[to_string(n)][gate_name].delay_list);
            int max_index = return_max_value(dp_table[to_string(n)][gate_name].delay_list);
            int area_min_index = return_min_value(dp_table[to_string(n)][gate_name].area_list);
            
            if(dp_table[to_string(n)][gate_name].delay_list[max_index]<require_time){
                selected_index = max_index;
            }
            else if(dp_table[to_string(n)][gate_name].delay_list[min_index]>require_time){
                selected_index = min_index;
            }
            else{
                float current_i = -1;
                float current_delay = -1;
                for (int i=0;i<dp_table[to_string(n)][gate_name].delay_list.size();i++){
                    if (dp_table[to_string(n)][gate_name].delay_list[i]<require_time){
                        if (dp_table[to_string(n)][gate_name].delay_list[i]>current_delay){
                            current_delay = dp_table[to_string(n)][gate_name].delay_list[i];
                            current_i = i;
                        }
                    }
                }
                selected_index = current_i;
            }
        }
        
        // selected_index = min_index;
        // if(gateHeightAssignmentMap[to_string(n)]=="8T"){
        //     selected_index = area_min_index;
        // }
        assert(selected_index != -1);
        // std::cout<<"index "<<selected_index<<"\n";
        
        

        for(int i=0; i<cut_list.size();i++){
           
            if (res->is_ci(cut_list[i]) || res->is_constant(cut_list[i])){
                continue;
            }
            auto fanin_gate_name = dp_table[to_string(n)][gate_name].pareto_gate_list[selected_index][i];
            // if(gateHeightAssignmentMap[to_string(cut_list[i])]=="8T"){
                
            //     float min_area = MAXFLOAT;
            //     for (auto g : dp_table[to_string(cut_list[i])]){
            //         if (g.second.area< min_area){
            //             min_area = g.second.area;
            //             fanin_gate_name = g.first;
            //         }
            //     }
            // }
            dp_back_propagation_rec(res, cut_list[i],n, fanin_gate_name,dp_iter_map,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog,lib_gates,gateHeightAssignmentMap);

               
            // dp_back_propagation_rec(res, cut_list[i],dp_table[to_string(n)][gate_name].best_gate_type_list[i]);
        }
    }

    void back_propagation_rec(binding_view<klut_network>* res, uint32_t n, uint32_t parent){
        // if(dp_iter_map[to_string(n)]>200){
        //     return;
        // }
        if(std::find(node_parent_map[to_string(n)].begin(),node_parent_map[to_string(n)].end(),parent)==node_parent_map[to_string(n)].end()){
            node_parent_map[to_string(n)].push_back(parent);
        }
        else{
            return;
        }
        std::cout<<"node "<<n<<" parent "<<parent<<"\n";
        vector<uint32_t> cut_list;
        dp_iter_map[to_string(n)]+=1;
        for(auto pair : dp_table[to_string(n)]){
            // std::cout<<pair.first<<"\n";
            for (auto l : *(dp_table[to_string(n)][pair.first].cut)){
                cut_list.push_back(l);
            }
            break;
        }
        for(int i=0; i<cut_list.size();i++){
            if (res->is_ci(cut_list[i]) || res->is_constant(cut_list[i])){
                continue;
            }
            
            back_propagation_rec(res, cut_list[i],n);
            // break;
        }
    }

    void back_propagation(binding_view<klut_network>* res,const std::vector<gate>& lib_gates){
        
        // std::cout<<"init dp_iter map\n";
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            
            dp_iter_map[to_string(n)] = 0;
        });
        
        res->foreach_po( [&]( auto const& o, uint32_t index ) {
            std::cout<<"back propagation, po "<<o<<"\n";
            back_propagation_rec(res,o,0);
        });
        // for(auto pair : dp_iter_map){
        //     std::cout<<pair.first<<" "<<pair.second<<"\n";
        // }
    }

    void dp_back_propagation_intuitive(binding_view<klut_network>* res,ot::Timer &timer,PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap,PlaceInfo::HashTable<std::string> &PlaceInfo_verilog,const std::vector<gate>& lib_gates,HashTable<std::string> &gateHeightAssignmentMap){
                topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
                    if(!res->is_ci(n)){
                        std::cout<<"processing "<<n<<"\n";
                        float require_time = *timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
                        if(dp_table[to_string(n)].size()==1){
                            dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)].begin()->second);
                        }
                        else{
                            
                            vector<string> gate_name_list = {};
                            vector<float> delay_list = {};
                            vector<float> area_list = {};
                            for ( auto gm : dp_table[to_string(n)]){
                                vector<string> fanin_solution_gate_name_list = {};
                                gate_name_list.push_back(gm.second.name);
                                for (auto l : *(gm.second.cut)){
                                    std::cout<<"solution size "<<dp_final_solution[to_string(l)].size()<<"\n";
                                    assert(dp_final_solution[to_string(l)].size()<=1);
                                    if(dp_final_solution[to_string(l)].size()==1){
                                        fanin_solution_gate_name_list.push_back(dp_final_solution[to_string(l)][0].name);
                                    }
                                    else{
                                        fanin_solution_gate_name_list.push_back(to_string(l));
                                    }
                                    
                                }
                                std::sort(fanin_solution_gate_name_list.begin(),fanin_solution_gate_name_list.end());
                                std::cout<<"solution fanin gate:\n";
                                for(auto g : fanin_solution_gate_name_list){
                                    std::cout<<g<<" ";
                                }
                                std::cout<<"\n";
                                bool found = false;
                                // std::cout<<gm.first<<" size of pareto_gate_list "<<gm.second.pareto_gate_list.size()<<"\n";
                                for(int i=0;i<gm.second.pareto_gate_list.size();i++){
                                    std::cout<<"see pareto_gate_list\n";
                                    for(auto g : gm.second.pareto_gate_list[i]){
                                        std::cout<<g<<"\n";
                                    }
                                    std::sort(gm.second.pareto_gate_list[i].begin(),gm.second.pareto_gate_list[i].end());
                                    if (fanin_solution_gate_name_list==gm.second.pareto_gate_list[i]){
                                        delay_list.push_back(gm.second.delay_list[i]);
                                        area_list.push_back(gm.second.area);
                                        found = true;
                                        break;
                                    }
                                }
                                if(!found){
                                    delay_list.push_back(gm.second.delay_list[0]);
                                    area_list.push_back(gm.second.area);
                                }
                                
                            }
                            std::cout<<"delay_list\n";
                            for(auto d : delay_list){
                                std::cout<<d<<" ";
                            }
                            std::cout<<"\n";
                            float require_time = *timer.report_rat("g"+to_string(n)+":"+node_to_gate_map[to_string(n)].output_name ,ot::MAX,ot::FALL);
        
                            int min_index = return_min_value(delay_list);
                            // int max_index = return_max_value(delay_list);
                            int max_index = return_min_value(area_list);
                 
                            int selected_index = -1;
                            if(delay_list[max_index]<require_time){
                                selected_index = max_index;
                            }
                            else if(delay_list[min_index]>require_time){
                                selected_index = min_index;
                            }
                            else{
                                float current_i = -1;
                                float current_delay = -1;
                                for (int i=0;i<delay_list.size();i++){
                                    if (delay_list[i]<require_time){
                                        if (delay_list[i]>current_delay){
                                            current_delay = delay_list[i];
                                            current_i = i;
                                        }
                                    }
                                }
                                selected_index = current_i;
                            }
                        assert(selected_index !=-1);
                            dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)][gate_name_list[selected_index]]);
                        }
                    }
                });
    }


    void dp_back_propagation(binding_view<klut_network>* res,ot::Timer &timer,PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap,PlaceInfo::HashTable<std::string> &PlaceInfo_verilog,const std::vector<gate>& lib_gates,HashTable<std::string> &gateHeightAssignmentMap){
         std::cout<<"enter propagation\n";
         float period = timer.clocks().at("ext_clk_virt").period();
        // initialize dp_final_solution
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            // std::cout<<"node "<<n<<"\n";
            
            if (!res->is_ci(n) && !res->is_constant(n)){
                dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)].begin()->second);
                
            }
            else{
                
            }
        });
        // HashTable<int> dp_iter_map;
        // std::cout<<"init dp_iter map\n";
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            
            dp_iter_map[to_string(n)] = 0;
        });

        // topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
        //     if(!res->is_ci(n)){
        //         float minimal_area = MAXFLOAT;
        //         float minimal_delay = MAXFLOAT;
        //         std::string area_select_gate;
        //         std::string delay_select_gate;
        //         for (auto g : dp_table[to_string(n)]){
        //             if(g.second.delay_list[0]<minimal_delay){
        //                 minimal_delay = g.second.delay_list[0];
        //                 delay_select_gate = g.first;
        //             }
        //             if (g.second.area<minimal_area){
        //                 minimal_area = g.second.area;
        //                 area_select_gate = g.first;
        //             }
        //         }
             

        //        dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)][delay_select_gate]);
              
                
        //     }
            
            
        // });
        res->foreach_po( [&]( auto const& o, uint32_t index ) {
            
          
            std::cout<<"back propagation, po "<<o<<"\n";
           if (o!=0){
            vector<string> gate_list;
            vector<float> delay_list;

            for (auto dp_gate : dp_table[to_string(o)]){
                gate_list.push_back(dp_gate.first);
                float best_delay = MAXFLOAT;
                for (auto d : dp_gate.second.delay_list){
                   
                    if(d<best_delay){
                        best_delay = d;
                        
                    }
           
                }
            delay_list.push_back(best_delay);
            }
            int min_index = return_min_value(delay_list);
            int max_index = return_max_value(delay_list);
            int selected_index = -1;
            if(delay_list[max_index]<period){
                selected_index = max_index;
            }
            else if(delay_list[min_index]>period){
                selected_index = min_index;
            }
            else{
                float current_i = -1;
                float current_delay = -1;
                for (int i=0;i<delay_list.size();i++){
                    if (delay_list[i]<period){
                        if (delay_list[i]>current_delay){
                            current_delay = delay_list[i];
                            current_i = i;
                        }
                    }
                }
                selected_index = current_i;
            }
            // selected_index = min_index;
            dp_back_propagation_rec(res,o,0,gate_list[selected_index],dp_iter_map,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog,lib_gates,gateHeightAssignmentMap);
           }
          
            
            
            
    
        
        } );


    }

    void naive_solution_selection(binding_view<klut_network>* res, std::string design_name, bool remap_flag, PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap){
        topo_view<binding_view<klut_network>>( *res ).foreach_node( [&]( auto n ) {
            
             if (!res->is_ci(n) && !res->is_constant(n)){
                std::cout<<"node "<<n<<"\n";
                
                if(dp_final_solution[to_string(n)].size()==0){ //for node which is not involved in a timing path
                    //TODO: delele the node
                    //current implementation : select the node with minimal area
                    float minimal_area =MAXFLOAT;
                    string best_gate;
                    for (auto g: dp_table[to_string(n)]){
                        if (g.second.area<minimal_area){
                            minimal_area = g.second.area;
                            best_gate = g.first;
                        }
                    }
                    dp_final_solution[to_string(n)].push_back(dp_table[to_string(n)][best_gate]);
                    
                }
                assert(dp_final_solution[to_string(n)].size()>0);
                if(dp_final_solution[to_string(n)].size()==1){
                    std::cout<<"node "<<n<<" with only one candidate\n";
                    auto ctr = 0u;
                    auto num_vars = dp_final_solution[to_string(n)][0].cut->size();
                    // std::cout<<"num_vars "<<num_vars<<" cut "<<*(dp_final_solution[to_string(n)][0].cut)<<"\n";
                    std::vector<signal_type> children( num_vars );
                    
        
                    for ( auto l : *(dp_final_solution[to_string(n)][0].cut) ){
                        if ( ctr >= num_vars )
                            break;
                        children[dp_final_solution[to_string(n)][0].permutation[ctr]] = l;
                        ctr++;   
                    }
                    
                        // std::cout<<"gate "<<n<<" found!\n";
                    PlaceInfoMap["g"+to_string(n)].cell_type = dp_final_solution[to_string(n)][0].name;
                    
                    
                    auto f = res->create_node_with_index(children, dp_final_solution[to_string(n)][0].gate_function, n);
                    res->add_binding( res->get_node( f ), dp_final_solution[to_string(n)][0].gate_id );
                }
                else{
                    std::cout<<"node "<<n<<" with more than one candidate\n";
                    int best_index=-1;
                    float best_area=MAXFLOAT;
                    for(int i=0;i<dp_final_solution[to_string(n)].size();i++){
                        if (dp_final_solution[to_string(n)][i].area < best_area){
                            best_area = dp_final_solution[to_string(n)][i].area;
                            best_index = i;
                        }
                    }
                   
                    auto ctr = 0u;
                    auto num_vars = dp_final_solution[to_string(n)][best_index].cut->size();
                    std::vector<signal_type> children( num_vars );
                    for ( auto l : *(dp_final_solution[to_string(n)][best_index].cut) ){
                        if ( ctr >= num_vars )
                            break;
                        children[dp_final_solution[to_string(n)][best_index].permutation[ctr]] = l;
                        ctr++;   
                    }
                    PlaceInfoMap["g"+to_string(n)].cell_type = dp_final_solution[to_string(n)][best_index].name;
                    auto f = res->create_node_with_index(children, dp_final_solution[to_string(n)][best_index].gate_function, n);
                    res->add_binding( res->get_node( f ), dp_final_solution[to_string(n)][best_index].gate_id );
                }
             }
        });
        write_verilog_with_binding(*res, "../examples/benchmarks/" + design_name + "/" + design_name +"_resynthesis_remap_"+to_string(remap_flag)+".v");
    }
    void dp_solve(binding_view<klut_network>* res,const std::vector<gate>& lib_gates,std::vector<gate>& gates,
                binding_cut_type& cuts,HashTable<std::string> &gateHeightAssignmentMap,std::string design_name,
                PlaceInfo::HashTable<PlaceInfo::Net> &PlaceInfo_net, PlaceInfo::HashTable<std::string> &PlaceInfo_gateToNet,
                PlaceInfo::HashTable<PlaceInfo::Gate> &PlaceInfoMap, PlaceInfo::HashTable<std::string> &PlaceInfo_verilog, bool remap_flag)
    {
        
        tech_library tech_lib( gates );

        // initialize opentimer object
        ot::Timer timer;
        std::string lib_file = "../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib";
        std::string verilog_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.v";
        std::string spef_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.spef";
        std::string sdc_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc";
        timer.read_celllib(lib_file)
             .read_verilog(verilog_file)
             .read_spef(spef_file)
             .read_sdc(sdc_file)
             .update_timing();
             
       
        // exit(1);
        
        
        init_dp_table(res,lib_gates,gates,cuts,gateHeightAssignmentMap, timer,remap_flag, PlaceInfoMap);
        // list the size of the dp table
        // int total_c = 0;
        // int large_c = 0;
        // for (auto entry : dp_table){
        //     total_c += 1;
        //     if(entry.second.size()>1){
        //         std::cout<<entry.first<<" size= "<<entry.second.size()<<"\n";
        //         large_c +=1;
        //     }
            
        // }
        // std::cout<<"total "<<total_c<<" large "<<large_c<<"\n";
      
        // back_propagation(res,lib_gates);
    
        // exit(1);
        std::cout<<"enter dp calculation\n";
        res->foreach_po( [&]( auto const& o, uint32_t index ) {
        // #pragma omp parallel for default(shared)
        vector<string> candidate_gate_names;
        for(auto gate_selection : dp_table[to_string(o)]){
            candidate_gate_names.push_back(gate_selection.first);
        }
        // #pragma omp parallel for default(shared)
        // for(int z =0;z<dp_table[to_string(o)].size();z++)
        // for(auto gate_selection : dp_table[to_string(o)]){
        for(auto candidate_gate : candidate_gate_names){
            // #pragma omp critical
                dp_solve_rec(res, o, candidate_gate, tech_lib, cuts, lib_gates,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog);
        }
        
        
      } );
    //   dp_back_propagation_intuitive(res,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog,lib_gates,gateHeightAssignmentMap);
    //   exit(1);
      dp_back_propagation(res,timer,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap,PlaceInfo_verilog,lib_gates,gateHeightAssignmentMap);
        std::optional<float> tns  = timer.report_tns();
    std::optional<float> wns  = timer.report_wns();
    std::optional<float> area = timer.report_area();
    std::cout <<"tns "<<*tns <<" wns "<<*wns<<" area "<<*area<<"\n";
    // std::cout <<"delay at gate 304 "<<*timer.report_at("g304:Z", ot::MAX, ot::FALL)<<"\n";
      
    
    }
}