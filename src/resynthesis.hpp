#pragma once

#include <mockturtle/mockturtle.hpp>

#include "./Hashtable.hpp"
#include "timing.hpp"
#include "PlaceInfo.hpp"

using namespace mockturtle;
using node_type = typename binding_view<klut_network>::node;
// using signal = uint64_t;
using signal_type = typename binding_view<klut_network>::signal;
using binding_cut_type = mockturtle::fast_network_cuts<binding_view<klut_network>, 5, true, mockturtle::empty_cut_data>;
using cut_t = cut_type<true, empty_cut_data>;
using cut_set_t = cut_set<cut_t, 50>;

template<typename T>
using HashTable = std::unordered_map<std::string , T>;

int resynthesis_single_node(binding_cut_type* cuts, uint32_t index,node_type node, const std::vector<supergate<4>>* original_super_gate,
                            const std::vector<gate>& lib_gates,std::vector<gate>& gates, binding_view<klut_network>* res,
                            binding_view<klut_network>* res_tmp, HashTable<std::string>& PlaceInfo_gateToNet, json* net_json_ptr,
                            std::string lib_file, std::string verilog_file, std::string spef_file, std::string sdc_file,
                            HashTable<PlaceInfo::Net>& PlaceInfo_net);

void update_json_file(const std::vector<std::string>& net_to_be_update, const std::vector<uint32_t>& original_cut,
                      const std::vector<uint32_t>& best_cut_list, const std::vector<std::string> original_element,
                      const std::vector<std::string> new_element, const std::string& gate_index,
                      json* net_json_ptr, std::vector<uint8_t> permutation);

