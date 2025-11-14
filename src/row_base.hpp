#ifndef _ROWBASE_H
#define _ROWBASE_H

// #include "omp.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <tuple>
#include <cmath>
#include <map>
#include "timing.hpp"
#include "kmeans.hpp"
// #include "Hashtable.hpp"

#include "../abacus/GlobalTimer/GlobalTimer.hpp"
#include "../abacus/Legalizer/Legalizer.hpp"
#include "../abacus/Parser/Parser.hpp"
#include "../abacus/Structure/Data.hpp"

template<typename T>
using HashTable = std::unordered_map<std::string , T>;


// void k_means(std::vector<std::pair<float, float>> pos, int k);
std::pair<std::string, std::string> score_row_height_baseline(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                                                    std::vector<int>& minor_coordinates, std::map<std::string, float>& minor_width,
                                                    std::vector<int>& chosen_centers ,int chosen_top_K, std::vector<int>& row_coordinates,
                                                    std::map<std::string, int>& minor_map,std::map<std::string, int>& major_map,
                                                    std::map<std::string, float>& major_width);

std::pair<std::string, std::string> score_row_height_dp(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                                                    std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, 
                                                    HashTable<std::string>& gateHeightAssignmentMap, HashTable<PlaceInfo::Gate>& PlaceInfoMap_modify);

std::vector<int> reassign_height(std::vector<int>& chosen_centers, std::vector<int> minor_coordinates, 
                                std::map<std::string, float>& minor_width, float threshold_minor, float threshold_major, int iter,
                                std::vector<int>& row_coordinates, HashTable<PlaceInfo::Gate>& PlaceInfoMap_modify,
                                std::map<std::string, int>& minor_map,std::map<std::string, int>& major_map,
                                std::map<std::string, float>& major_width, std::vector<int>& reassign_major_rows);

float find_closest_row(int gate_y, const std::vector<int>& major_rows, std::map<int, float>& major_row_width_sum,
                     float gate_width, float threshold); 

std::vector<float> kmeans(std::vector<int> minor_coordinate, std::vector<int> chosen_centers, 
                        int num_clusters, int iter); 

std::string replace_variable_name(const std::string& original_name, const std::string& old_part, const std::string& new_part); 

void row_base_solver(std::string design_name, HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                    std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, std::pair<std::string, 
                    std::string>& target, HashTable<std::string>& gateHeightAssignmentMap, std::string face,
                    int chosen_top_K, int iter, int density);

void abacus_legalize(std::string design_name,std::vector<int> reassign_minor_rows,std::vector<int> reassign_major_rows,std::pair<std::string, std::string>& targe,string phase, bool remap_flag);

void row_base_report_timing(std::string design_name, HashTable<std::string> PlaceInfo_verilog, std::string phase,int remap_flag);
void read_row_base_files(const std::string& design_name, HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Net>& PlaceInfo_net,
                         HashTable<PlaceInfo::Pin>& PlaceInfo_pin, HashTable<std::string>& PlaceInfo_gateToNet, std::string phase, int remap_flag);

#endif