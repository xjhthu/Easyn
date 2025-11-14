#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "resynthesis.hpp"
#include <omp.h>
#include <mutex>

using namespace mockturtle;
using json = nlohmann::json;

float max_delay(const std::array<float, 4>& delay_array) {
    return  *(std::max_element((std::begin(delay_array)), (std::end(delay_array)))); 
}

// float cut_evaluate(const std::vector<supergate<4>>* supergate,binding_view<klut_network>* res,uint32_t index, node_type node, const std::vector<gate>& lib_gates,binding_view<klut_network>* res_tmp){

// //    auto original_gate  = lib_gates[res.get_binding_index( node )];
//    float min_delay = 99999.0;
//    auto selected_gate = (*supergate)[0];
//    for ( auto const& gate : *supergate ){
//     auto gate_name = lib_gates[gate.root->id].name;
//     auto delay = max_delay(gate.tdelay);
//     std::cout << "gate name "<<gate_name<<" delay: " << delay<<"\n";
//     if (delay < min_delay){
//         min_delay = delay;
//         selected_gate = gate;
//     }
//     std::cout<<"\n";

//     // remap/resize
//     std::vector<signal_type> children( selected_gate.root->num_vars );
//     // std::vector<signal<klut_network>> children( selected_gate.root->num_vars );
//     res_tmp->remove_binding(node);
    
//    }
//   return 0.0;
// }

/*
int resynthesis_single_node_parallel(binding_cut_type* cuts, uint32_t index,node_type node, const std::vector<supergate<4>>* original_super_gate,
                            const std::vector<gate>& lib_gates,std::vector<gate>& gates, binding_view<klut_network>* res,
                            binding_view<klut_network>* res_tmp, HashTable<std::string>& PlaceInfo_gateToNet, json* net_json_ptr,
                            std::string lib_file, std::string verilog_file, std::string spef_file, std::string sdc_file,
                            HashTable<PlaceInfo::Net>& PlaceInfo_net, ot::Timer& timer_greedy)
{
    auto & node_cuts = (*cuts).cuts( index );
    float min_delay = 99999.0;
    // mockturtle::supergate<4>* selected_gate;
    cut_t* best_cut;
    kitty::dynamic_truth_table gate_function;
    int num_vars = -1;
    std::vector<uint8_t> permutation;
    uint32_t gate_id = -1;
    std::string gate_name;
    std::vector<uint32_t> original_cut;
    std::vector<uint32_t> best_cut_list;
    std::vector<std::string> net_to_be_update;
    
    res->foreach_fanin(node,[&](auto fanin){
        original_cut.push_back(fanin);
      });
   
    tech_library tech_lib( gates );
 
    // Greedy algorithm: Find the best results for gate in all cuts
    for ( auto& cut : node_cuts ){
        std::cout << "\n\n"<<"start one cut of the node"<<std::endl;
        std::cout<< *cut <<" "<< *cut->begin()<<" "<<index<<std::endl;
        if ((cut->size()&&*cut->begin() == index) ==1 || cut->size()>4){
            continue;
        }
        
        auto tt = (*cuts).truth_table( *cut );
        auto fe = kitty::shrink_to<4>( tt );
        auto supergates_pos = tech_lib.get_supergates( fe );

        // Find the member of the connection corresponding to the best cut
        std::vector<uint32_t> new_cut;
        for ( auto l : *cut ){
            new_cut.push_back(l);
        }
        if (supergates_pos !=nullptr){
            
            int gate_index = 0;
            std::unordered_map<int, double> gate_delay_map; // Use thread-safe container
            std::cout<<"Start multi-thread computing!" << std::endl;
            omp_lock_t lock;
            omp_init_lock(&lock);
            #pragma omp parallel 
            {
                std::unordered_map<int, double> private_gate_delay_map; // Private copy of gate_delay_map for each thread
                
                #pragma omp for
                for ( auto &gate : *supergates_pos ){
                    if (unsigned(gate.polarity)!=0){ // skip nodes which require negation
                        continue;
                    }

                    permutation = gate.permutation; 
                    gate_id = gate.root->id;
                    gate_name = lib_gates[gate.root->id].name;

                    createRCTree* tree_res = new createRCTree(PlaceInfo_net, PlaceInfo_gateToNet);
                    
                    FluteResult result;
                    if (original_cut != new_cut){
                        tree_res->buildTopo_greedy(lib_file, verilog_file, spef_file, sdc_file, 
                                                    original_cut, new_cut, gate_id, gate_name, permutation);
                        result = tree_res->runFlute();  // results for the entire circuit after replacing wns and tns //TODO:modify
                    }
            
                    delete tree_res;

                    // auto delay = max_delay(gate.tdelay); 
                   
                    omp_set_lock(&lock);
                    std::cout << "thread_num:" << omp_get_thread_num() << "\n";
                    std::cout << "candidate gate name " << lib_gates[gate.root->id].name 
                            << " POLARITY: " << unsigned(gate.polarity) << " wns:" << result.wns
                            << " tns:" << result.tns << "\n" << "\n";

                    private_gate_delay_map[gate_index++] = result.tns;
                    omp_unset_lock(&lock);            
                }
                #pragma omp critical
                {
                    // Merge private_gate_delay_map of each thread into gate_delay_map
                    gate_delay_map.insert(private_gate_delay_map.begin(), private_gate_delay_map.end());
                }
            
            }
            
            auto it_min_delay = std::min_element(gate_delay_map.begin(), gate_delay_map.end(),
                [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                    return a.second < b.second;
                });
            
            if (it_min_delay->second < min_delay){
                auto& gate = (*supergates_pos)[it_min_delay->first];
                min_delay = it_min_delay->second;
                gate_function  = gate.root->function;
                best_cut = cut;
                num_vars = gate.root->num_vars;
                permutation = gate.permutation; 
                gate_id = gate.root->id;    //TODO: check the type of the variables
                gate_name = lib_gates[gate.root->id].name;
                std::cout << "Gate with the smallest delay: " << gate_name << std::endl;
            } else continue;
        
        }

    }
    omp_destroy_lock(&lock);
    std::cout<<"\n\n"<<"Finish multi-thread computing!"<<std::endl;

    // Finally find the one with the smallest delay among all the corresponding replacement gates for all cuts
    //remap/resizing
    if (num_vars==-1){
        return 0;
    }
    std::cout<<"min delay: "<<min_delay<<"\n";
    std::vector<signal_type> children( num_vars );

    auto ctr = 0u;
    // Find the member of the connection corresponding to the best cut
    for ( auto l : *best_cut ){
        best_cut_list.push_back(l);
        std::cout<<l<<" ";
        if ( ctr >= num_vars )
            break;
        children[permutation[ctr]] = l;
        ctr++;   
    }

    // determine the net to be re-constructed
    // If a gate exists in both the old and new cut, do not modify its net.
    sort(original_cut.begin(),original_cut.end());
    sort(best_cut_list.begin(),best_cut_list.end());
    std::vector<std::string> original_element, new_element;
    if (original_cut != best_cut_list){
        for (const auto& n : original_cut) {
            original_element.emplace_back("g" + std::to_string(n));
            net_to_be_update.emplace_back(PlaceInfo_gateToNet["g" + std::to_string(n)]);
        }
        for (const auto& n : best_cut_list) {
            new_element.emplace_back("g" + std::to_string(n));
            net_to_be_update.emplace_back(PlaceInfo_gateToNet["g" + std::to_string(n)]);
        }
    }

    std::string gate_chosen = "g" + std::to_string(gate_id);
    timer_greedy.remove_gate(gate_chosen);
    timer_greedy.insert_gate(gate_chosen, gate_name); 
    {

        std::string gate_chosen_net = PlaceInfo_gateToNet[gate_chosen];
        timer_greedy.remove_net(gate_chosen_net);
        auto& net = PlaceInfo_net[gate_chosen_net];
        for (auto& pin : net.pins) {
            if (pin.type != "standardcell"){
                timer_greedy.connect_pin(pin.name, gate_chosen_net);
            }else{
                timer_greedy.connect_pin(pin.cellname + ":" + pin.name, gate_chosen_net); 
            }
        }

    }
    
    for (const auto& net_name: net_to_be_update){
        auto& net = PlaceInfo_net[net_name];

        // Adding flags to determine the net attribute.
        bool contains_orig_only = false;
        bool contains_new_only = false;
        bool has_common_gates = false;

        for (const auto& pin : net.pins) {
            if (pin.name == "ZN" || pin.name == "Z"){

                auto it_orig = std::find(original_element.begin(), original_element.end(), pin.cellname);
                auto it_new = std::find(new_element.begin(), new_element.end(), pin.cellname);
                if (it_orig != original_element.end() && it_new != new_element.end()) {
                    has_common_gates = true;
                } else if (it_orig != original_element.end() && it_new == new_element.end()) {
                    contains_orig_only = true;
                } else if (it_orig == original_element.end() && it_new != new_element.end()) {
                    contains_new_only = true;
                } else {
                    std::cerr << "Error occurs!" << std::endl;
                    break;
                }
            }
        }

        timer->remove_net(net_name);
        net_names.emplace_back(net_name);
        timer->insert_net(net_name);

        for (auto& pin : net.pins) {
            if (pin.type != "standardcell"){
                timer->connect_pin(pin.name, net_name);
            } else {
                if (pin.cellname == gate_chosen){
                    if (contains_orig_only){
                        continue;
                    }else {  //has_common_gates 
                        std::string permu;
                        for (auto& pair : permutation_map){
                            if (pair.second != 1){
                                permu = pair.first;
                                pair.second = 1;
                                break;
                            }
                        }
                        timer->connect_pin(gate_chosen +":" + permu, net_name);
                        continue;
                    }
                    
                } else{
                    timer->connect_pin(pin.cellname + ":" + pin.name, net_name);
                }
            }
        }

        if (contains_new_only){
            std::string permu;
            for (auto& pair : permutation_map){
                if (pair.second != 1){
                    permu = pair.first;
                    pair.second = 1;
                    break;
                }
            }
            timer->connect_pin(gate_chosen +":" + permu, net_name);
        }

    }

    timer->update_timing();
    //TODO: use the best gate to modify the global_tree
    //timer_greedy->remove_gate();...

    auto f = res->create_node_with_index(children, gate_function, node);
    res->add_binding( res->get_node( f ), gate_id );
    std::cout<<"\n";
    std::cout << "new gate id "<<res->get_node( f )<< " gate name "<<gate_name<<std::endl;
    std::cout<<"net to be updated:\n";
    for (auto n:net_to_be_update){
        std::cout<<n<<" ";
    }
    std::cout<<"\n";
    std::cout<<"--------------------"<<std::endl;

    return 0;
}

void update_json_file(const std::vector<std::string>& net_to_be_update, const std::vector<uint32_t>& original_cut,
                      const std::vector<uint32_t>& best_cut_list, int node_index,
                      json* net_json_ptr, std::vector<uint8_t> permutation) 
{  

    for (auto& net : (*net_json_ptr)){
        const auto& net_name = net["name"];
        if (std::find(net_to_be_update.begin(), net_to_be_update.end(), net_name) == net_to_be_update.end()) {
            continue;
        }
        
        // Adding flags to determine the net attribute.
        bool contains_orig_only = false;
        bool contains_new_only = false;
        bool has_common_gates = false;

        for (auto& pin : net["pins"]){
            if (pin["type"] != "standardcell" || (pin["name"] != "ZN" && pin["name"] != "Z")) {
                continue;
            }

            auto it_orig = std::find(original_element.begin(), original_element.end(), pin["cellname"]);
            auto it_new = std::find(new_element.begin(), new_element.end(), pin["cellname"]);
            if (it_orig != original_element.end() && it_new != new_element.end()) {
                has_common_gates = true;
            } else if (it_orig != original_element.end() && it_new == new_element.end()) {
                contains_orig_only = true;
            } else if (it_orig == original_element.end() && it_new != new_element.end()) {
                contains_new_only = true;
            } else 
                break;
        
        }
        
        if (has_common_gates) {
            continue;
        }

        // Update nets
        if (contains_orig_only) {
            auto& pins = net["pins"];
            auto it = std::find_if(pins.begin(), pins.end(),
                [&gate_index](const auto& pin) { return pin.contains("cellname") && pin["cellname"] == gate_index; });
            if (it != pins.end()) {
                pins.erase(it);
                break;
            }
        }
        else if (contains_new_only) {
            auto& pins = net["pins"];
            pins.push_back({
                {"cellname", gate_index},
                {"name", "A1"}, //TODO: pin_name
                {"type", "standardcell"}
            });
        }else
            continue;
        
    }       
    return;
}
*/
    
        
       