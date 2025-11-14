#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>
#include <limits>
#include <algorithm>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "row_base.hpp"

using namespace KMEANS;

std::pair<std::string, std::string> score_row_height_baseline(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                                                    std::vector<int>& minor_coordinates, std::map<std::string, float>& minor_width,
                                                    std::vector<int>& chosen_centers ,int chosen_top_K, std::vector<int>& row_coordinates,
                                                    std::map<std::string, int>& minor_map,std::map<std::string, int>& major_map,
                                                    std::map<std::string, float>& major_width)
{
    std::unordered_map<int, std::vector<std::string>> gateNamesByY;
    std::unordered_map<int, std::string> rowHeightByY;
    std::unordered_map<int, float> rowScoreByY;
    std::unordered_map<std::string, std::string> gateInfo;
    float totalArea8T = 0;
    float totalArea12T = 0;

    // Count the gate name and cell_type for each y value and 
    // calculate the height of each row based on the number of cell_types
    for (const auto& gate : PlaceInfoMap) {
        int y = gate.second.y;
        gateNamesByY[y].push_back(gate.second.name);
        std::string cell_type = gate.second.cell_type;
        if (cell_type.find("8T") != std::string::npos) {
            gateInfo[gate.second.name] = "8T";
            if (rowHeightByY.find(y) == rowHeightByY.end()) {
                rowHeightByY[y] = "8T";
            } else {
                int count8T = 0;
                int count12T = 0;
                for (const auto& name : gateNamesByY[y]) {
                    std::string cell_type = PlaceInfoMap[name].cell_type;
                    if (cell_type.find("8T") != std::string::npos) {
                        count8T++;
                    } else if (cell_type.find("12T") != std::string::npos) {
                        count12T++;
                    }
                }
                if (count8T >= count12T) {
                    rowHeightByY[y] = "8T";
                } else {
                    rowHeightByY[y] = "12T";
                }
            }
            totalArea8T += PlaceInfo_macro[cell_type].area;
        }
        else if (cell_type.find("12T") != std::string::npos) {
            gateInfo[gate.second.name] = "12T";
            if (rowHeightByY.find(y) == rowHeightByY.end()) {
                rowHeightByY[y] = "12T";
            } else {
                int count8T = 0;
                int count12T = 0;
                for (const auto& name : gateNamesByY[y]) { 
                    std::string cell_type = PlaceInfoMap[name].cell_type;
                    if (cell_type.find("8T") != std::string::npos) {
                        count8T++;
                    } else if (cell_type.find("12T") != std::string::npos) {
                        count12T++;
                    }
                }
                if (count12T >= count8T) {
                    rowHeightByY[y] = "12T";
                } else {
                    rowHeightByY[y] = "8T";
                }
            }
            totalArea12T += PlaceInfo_macro[cell_type].area;
            std::cout<<cell_type<<' '<<PlaceInfo_macro[cell_type].area<<std::endl;
        }
    }

    std::string target_minor = totalArea12T > totalArea8T? "8T" : "12T";
    std::string target_major = totalArea12T < totalArea8T? "8T" : "12T";
    for (const auto& gate : PlaceInfoMap) {
        int y = gate.second.y;
        std::string cell_type = gate.second.cell_type;

        if (cell_type.find(target_minor) != std::string::npos) {
            minor_coordinates.push_back(y);
            minor_map[gate.second.name] = y;
            minor_width[gate.second.name] = PlaceInfo_macro[cell_type].width;
        } 

        if (cell_type.find(target_major) != std::string::npos) {
            major_map[gate.second.name] = y;
            major_width[gate.second.name] = PlaceInfo_macro[cell_type].width;
        } 
    }

    // Calculate the score for each row
    for (const auto& [y, gateNames] : gateNamesByY) {
        float areaSameHeight = 0;
        float areaDiffHeight = 0;
        std::string rowHeight = totalArea8T > totalArea12T ? "12T" : "8T";
        for (const auto& name : gateNames) {
            std::string cell_type = PlaceInfoMap[name].cell_type;
            if (cell_type.find(rowHeight) != std::string::npos) {
                areaSameHeight += PlaceInfo_macro[cell_type].area;
            }
            else {
                areaDiffHeight += PlaceInfo_macro[cell_type].area;
            }
        }
        rowScoreByY[y] = areaSameHeight - areaDiffHeight;
    }

    // Sort the rows according to the row score
    // Create a vector of pairs for sorting
    std::vector<std::pair<int, float>> rowScorePairs;
    for (const auto& [y, gateNames] : gateNamesByY) {
        rowScorePairs.emplace_back(y, rowScoreByY[y]);
        row_coordinates.emplace_back(y);
    }

    // Sort the row scores in descending order
    std::sort(rowScorePairs.begin(), rowScorePairs.end(),
            [](const auto& lhs, const auto& rhs) {
                return lhs.second > rhs.second;
            });
    for (int i = 0; i < chosen_top_K; i++){
        if (i >= rowScorePairs.size()){
            break;
        }
        chosen_centers.push_back(rowScorePairs[i].first);
        std::cout<<"center "<<rowScorePairs[i].first<<std::endl;
    }

    // // Output results sorted by row score
    // for (const auto& rowScorePair : rowScorePairs) {
    //     std::cout << "y = " << rowScorePair.first << ": ";
    //     for (const auto& name : gateNamesByY[rowScorePair.first]) {
    //         std::cout << name << "("<<gateInfo[name]<<")"<<" ";
    //     }
    //     std::cout << std::endl;

    //     std::cout << "Row major height: " << rowHeightByY[rowScorePair.first] << std::endl;
    //     std::cout << "Row score: " << rowScorePair.second << std::endl;

    //     std::cout << std::endl;
    // }

    std::cout << "Total Area (8T cell): " << totalArea8T << std::endl;
    std::cout << "Total Area (12T cell): " << totalArea12T << std::endl;
    std::cout << "The cell type with larger total area is: "
                << (totalArea8T > totalArea12T ? "8Tcell" : "12Tcell") <<std::endl;
    
    return std::make_pair(target_minor,target_major);
    
}

std::pair<std::string, std::string> score_row_height_dp(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                                                    std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, 
                                                    HashTable<std::string>& gateHeightAssignmentMap, HashTable<PlaceInfo::Gate>& PlaceInfoMap_modify)
{
    std::unordered_map<int, std::vector<std::string>> gateNamesByY;
    std::unordered_map<int, float> rowScoreByY;
    std::vector<int> row_coordinates;
    float totalArea8T = 0;
    float totalArea12T = 0;

    // Count the gate name and cell_type for each y value and 
    // calculate the height of each row based on the number of cell_types
    for (const auto& gate : PlaceInfoMap) {
        int y = gate.second.y;
        gateNamesByY[y].push_back(gate.second.name);
        std::string cell_type_origin = gate.second.cell_type;
        std::string type_name = gateHeightAssignmentMap[gate.second.name.substr(1)];
        std::string cell_type_resynthesis;

        if (cell_type_origin.find("8T") != std::string::npos && type_name != "8T") {
            cell_type_resynthesis = replace_variable_name(cell_type_origin, "8T", "12T");
        } else if (cell_type_origin.find("12T") != std::string::npos && type_name != "12T") {
            cell_type_resynthesis = replace_variable_name(cell_type_origin, "12T", "8T");
        } else {
            cell_type_resynthesis = cell_type_origin;
        }

        PlaceInfoMap_modify[gate.second.name].cell_type = cell_type_resynthesis;

        // Assign the cells to the row
        if (type_name == "8T") {
            totalArea8T += 1;
        }
        else if (type_name == "12T") {
            totalArea12T += 1;
        }
    }

    std::string target_minor = totalArea12T > totalArea8T? "8T" : "12T";
    std::string target_major = totalArea12T < totalArea8T? "8T" : "12T";

    // Calculate the score for each row
    for (const auto& [y, gateNames] : gateNamesByY) {
        float areaSameHeight = 0;
        float areaDiffHeight = 0;
        std::string rowHeight = target_minor;
        for (const auto& name : gateNames) {
            std::string cell_type = PlaceInfoMap_modify[name].cell_type;
            if (cell_type.find(rowHeight) != std::string::npos) {
                areaSameHeight += 1;
            }
            else {
                areaDiffHeight += 1;
            }
        }
        rowScoreByY[y] = areaSameHeight - areaDiffHeight;
    }

    for (const auto& pair: rowScoreByY){
        if (pair.second > 0.0){
            reassign_minor_rows.push_back(pair.first);
        } else{
            reassign_major_rows.push_back(pair.first);
        }
    } 

    // Output results sorted by row score
    for (const auto& pair : rowScoreByY) {
        std::cout << "y = " << pair.first << ": ";
        for (const auto& name : gateNamesByY[pair.first]) {
            std::cout << name << "("<<gateHeightAssignmentMap[name.substr(1)]<<")"<<" ";
        }
        std::cout << std::endl;
        std::cout << "Row score: " << pair.second << std::endl;

    }

    std::cout << "Total Area (8T cell): " << totalArea8T << std::endl;
    std::cout << "Total Area (12T cell): " << totalArea12T << std::endl;
    std::cout << "The cell type with larger total area is: "
                << (totalArea8T > totalArea12T ? "8Tcell" : "12Tcell") <<std::endl;
    
    return std::make_pair(target_minor,target_major);
    
}

std::string replace_variable_name(const std::string& original_name, const std::string& old_part, const std::string& new_part) {
    std::string replaced_name = original_name;
    size_t pos = replaced_name.find(old_part);
    if (pos != std::string::npos) {
        replaced_name.replace(pos, old_part.length(), new_part);
    }
    return replaced_name;
}

std::vector<int> reassign_height(std::vector<int>& chosen_centers, std::vector<int> minor_coordinates, 
                                std::map<std::string, float>& minor_width, float threshold_minor, float threshold_major,int iter,
                                std::vector<int>& row_coordinates, HashTable<PlaceInfo::Gate>& PlaceInfoMap_modify,
                                std::map<std::string, int>& minor_map,std::map<std::string, int>& major_map,
                                std::map<std::string, float>& major_width, std::vector<int>& reassign_major_centers)
{
    float W_sum = 0.0;
    for (const auto& width : minor_width){
        W_sum += width.second;
    }
    float W_average = W_sum / chosen_centers.size();

    std::vector<int> reassign_centers;
    std::vector<int> current_centers(chosen_centers.size());

    std::transform(chosen_centers.begin(), chosen_centers.end(), current_centers.begin(),
        [](int val) { return static_cast<float>(val); });

    int num_clusters = chosen_centers.size();
    std::cout<<"here?\n";
    while (true){
        std::cout<<"enter while\n";
        bool flag = 1;
        std::vector<int> reassign_centers_tmp;
        std::map<std::string, int> minor_map_copy = minor_map;
        std::cout<<"before kmeans"<<num_clusters<<' '<<iter<<endl;
            for(auto i:minor_coordinates)cout<<"minor_coordinates "<<i<<endl;
            for(auto i:current_centers)cout<<"current_centers "<<i<<endl;
        std::vector<float> kmeans_centers = kmeans(minor_coordinates, current_centers, num_clusters, iter);
        std::cout<<"before get center\n";
        // Find the row that is most similar to kmeans_centers
        for (auto center : kmeans_centers) {
            float min_dist = std::numeric_limits<float>::max();
            int closest_row = 0;
            for (auto row : row_coordinates) {
                float dist = std::abs(center - row);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_row = row;
                }
            }
            reassign_centers_tmp.push_back(closest_row);
        }
       
        // Update y-coordinates in minor_map and get gate names
        std::map<int, float> row_width_sum;
        for (auto& entry : minor_map_copy){
            std::string gate_name = entry.first;
            int gate_y = entry.second;
            float min_dist = std::numeric_limits<float>::max();
            int closest_row = 0;
            for (auto row : reassign_centers_tmp){
                float dist = std::abs(gate_y - row);
                if (dist < min_dist){
                    min_dist = dist;
                    closest_row = row;
                }
            }
            entry.second = closest_row;
            row_width_sum[closest_row] += minor_width[gate_name];
        }
        
        // for (const auto& [key, value] : row_width_sum) {
        //     std::cout << key << ": " << value << std::endl;
        // }

        // Reallocate the rows that have a total width larger than threshold * W_average
        for(auto& center : reassign_centers_tmp){
            if (row_width_sum[center] > threshold_minor * W_average){
                // Find the nearest row to the current centre in row_coordinates 
                float new_center_up = center; 
                float new_center_below = center; 
                float min_dist_up = std::numeric_limits<float>::max();
                float min_dist_below = std::numeric_limits<float>::max();
                for (auto row : row_coordinates){
                    // Check if the row is already assigned as a center
                    if (std::find(reassign_centers_tmp.begin(), reassign_centers_tmp.end(), row) == reassign_centers_tmp.end()){
                        if (row < center){
                            float dist = std::abs(center - row);
                            if (dist < min_dist_up){
                                min_dist_up = dist;
                                new_center_up = row;
                            }
                        } else if (row > center) {
                            float dist = std::abs(center - row);
                            if (dist < min_dist_below){
                                min_dist_below = dist;
                                new_center_below = row;
                            }
                        }
                    }
                }
               
                if (new_center_up != center && new_center_below != center) {
                    reassign_centers_tmp.erase(std::remove(reassign_centers_tmp.begin(), reassign_centers_tmp.end(), center), reassign_centers_tmp.end());
                    reassign_centers_tmp.push_back(new_center_up);
                    reassign_centers_tmp.push_back(new_center_below);
                } else if (new_center_up != center ) {
                    reassign_centers_tmp.push_back(new_center_up);   
                } else if (new_center_below != center) {
                    reassign_centers_tmp.push_back(new_center_below);
                } else {
                    // No new center found, do nothing
                    std::cout << "No new center found." << std::endl;
                }
                num_clusters += 1;
                current_centers = reassign_centers_tmp;
                flag = false;
            }
        }

        if (flag){
            reassign_centers = reassign_centers_tmp;
            std::sort(reassign_centers.begin(), reassign_centers.end());
            auto last = std::unique(reassign_centers.begin(), reassign_centers.end());
            reassign_centers.erase(last, reassign_centers.end());
            break;
        }
    }
    
    // Find the major cell on reassign_centers
    std::vector<int> major_rows;
    std::map<int, float> major_row_width_sum;
    for (auto row : major_rows) {
        major_row_width_sum[row] = 0.0;
    }
    for (auto row : row_coordinates) {
        if (std::find(reassign_centers.begin(), reassign_centers.end(), static_cast<float>(row)) == reassign_centers.end()) {
            major_rows.push_back(static_cast<float>(row));
        }
    }

    float W_sum_major = 0.0;
    for (const auto& width : major_width){
        W_sum_major += width.second;
    }
    float W_average_major = W_sum_major / major_rows.size();
    
    // Count major_row_width_sum
    for (const auto& [gate_name, y_coord] : major_map){
        auto width_it = major_width.find(gate_name);
        if (width_it == major_width.end()){
            continue;
        }
        float width = width_it->second;

        // Only count the cells on major rows
        if (std::find(major_rows.begin(), major_rows.end(), y_coord) != major_rows.end()) {
            // std::cout << "y_coord:"<<y_coord<<std::endl;
            auto row_it = major_row_width_sum.find(y_coord);
            if (row_it == major_row_width_sum.end()){
                major_row_width_sum[y_coord] = width;
            } else {
                row_it->second += width;
            }
            // if (row_it->second > threshold_major * W_average_major) {
            //     std::cerr << "Error: Major Row " << y_coord << " exceeds the threshold_major of " << threshold_major << " times average row width!" << std::endl;
            //     exit(1);
            // }
        }
    }

    std::map<std::string, int> major_update;
    std::map<std::string, int> major_exceed;
    for (auto& entry : major_map){
        std::string gate_name = entry.first;
        int gate_y = entry.second;
        if (std::find(reassign_centers.begin(), reassign_centers.end(), static_cast<float>(gate_y)) != reassign_centers.end()) {
            auto width_it = major_width.find(gate_name);
            float gate_width = width_it->second;
            
            // Find the nearest row that can meet the threshold
            float chosen = find_closest_row(gate_y, major_rows, major_row_width_sum, gate_width, threshold_major * W_average_major*0.9);
            if (chosen == -1){
                major_exceed[gate_name] = gate_y;
                continue;
            }else{
                if (chosen != gate_y){
                    major_row_width_sum[chosen] += gate_width;
                    major_update[gate_name] = chosen;
                }
            }
        } 
    }
    for (auto& entry : major_exceed){
        std::string gate_name = entry.first;
        int gate_y = entry.second;

        auto width_it = major_width.find(gate_name);
        float gate_width = width_it->second;
        
        // Find the nearest row that can meet the threshold
        float chosen = find_closest_row(gate_y, major_rows, major_row_width_sum, gate_width, threshold_major * W_average_major * 1.1);
        if (chosen == -1){
            std::cerr << "Error: Please modify the threshold for the major row!" << std::endl;
            exit(1);
        }else{
            if (chosen != gate_y){
                major_row_width_sum[chosen] += gate_width;
                major_update[gate_name] = chosen;
            }
        }
        
    }

    std::cout <<"Therhold!:"<< W_average_major* threshold_major<<std::endl;
    for (const auto& [key, value] : major_row_width_sum) {
        std::cout << key << ": " << value << std::endl;
    }
    
    // Minor cells modify
    for (auto& entry : minor_map){
        std::string gate_name = entry.first;
        int gate_y = entry.second;

        float min_dist = std::numeric_limits<float>::max();
        int closest_row = 0;
        for (auto row : reassign_centers){
            float dist = std::abs(gate_y - row);
            if (dist < min_dist){
                min_dist = dist;
                closest_row = row;
            }
        }
        entry.second = closest_row;
    }
        
    // Update PlaceInfoMap_modify
     for (auto& gate_pair : PlaceInfoMap_modify) {
        auto coord_it = minor_map.find(gate_pair.first);
        if (coord_it != minor_map.end()){
            gate_pair.second.y = coord_it->second;
        }
    }
    for (auto& gate_pair : PlaceInfoMap_modify) {
        auto coord_it = major_update.find(gate_pair.first);
        if (coord_it != major_update.end()){
            gate_pair.second.y = coord_it->second;
        }
    }

    reassign_major_centers = major_rows;
    return reassign_centers;
}

float find_closest_row(int gate_y, const std::vector<int>& major_rows, std::map<int, float>& major_row_width_sum,
                     float gate_width, float threshold) 
{
    
    // Iterate through all major rows to find the nearest row
    float closest_row = major_rows.front();
    float closest_distance = std::abs(gate_y - closest_row);
    for (int row : major_rows) {
        float distance = std::abs(gate_y - row);
        if (distance < closest_distance) {
            closest_distance = distance;
            closest_row = row;
        }
    }
    
    // Recursively search all feasible rows if the load of the nearest row is greater than the threshold
    if (major_row_width_sum[closest_row] + gate_width > threshold) {
        std::vector<int> feasible_rows;
        for (int row : major_rows) {
            if (major_row_width_sum[row] + gate_width <= threshold) {
                feasible_rows.push_back(row);
            }
        }
        
        // If there is no feasible row, return -1 to indicate that no suitable row was found
        if (feasible_rows.empty()) {
            // std::cerr << "Error: All candidate Major Row exceeds the threshold_major of " << threshold_major << " times average row width!" << std::endl;
            return -1;
            
        }
        
        // Recursive search in all feasible rows
        closest_row = find_closest_row(gate_y, feasible_rows, major_row_width_sum, gate_width, threshold);
        
    }
    
    return closest_row;

}

// //Input: pos : The coordinates of each cell on the row
// //        k    : number of clusters
// void k_means(std::vector<std::pair<float, float>> pos, int k)
// {

//     int pointId = 1;
//     std::vector<Point> all_points;
//     for (int i = 0; i < pos.size(); i++){
//         float x = pos[i].first;
//         float y = pos[i].second;
//         std::vector<float> v = {x, y};
//         Point p(pointId++, v);
//         all_points.push_back(p);
//     }
    
//     // Return if number of clusters > number of points
//     if ((int)all_points.size() < k)
//     {
//         std::cerr << "Error: Number of clusters greater than number of points." << std::endl;
//         return;
//     }

//     // Running K-Means Clustering
//     int iters = 100;
//     std::unordered_map<int, std::vector<float>> clusterMap;
//     KMeans kmeans(k, iters);
//     clusterMap = kmeans.run(all_points);

//     return;
// }


std::vector<float> kmeans(std::vector<int> minor_coordinate, std::vector<int> chosen_centers, 
                        int num_clusters, int iter) 
{   
    std::vector<int> cluster_assignment(minor_coordinate.size());  // Record which cluster each point belongs to
    std::vector<int> cluster_size(num_clusters);          // Record the number of points contained in each cluster
    std::vector<int> cluster_sum(num_clusters);  // Record the sum of the coordinates of the points in each cluster
    std::vector<float> cluster_center(num_clusters);

    // Information on initializing clusters
    for (int i = 0; i < minor_coordinate.size(); i++) {
        float best_cluster = 0;
        float best_distance = abs(minor_coordinate[i] - chosen_centers[0]);
        for (int j = 1; j < chosen_centers.size(); j++) {
            float d = abs(minor_coordinate[i] - chosen_centers[j]);
            if (d < best_distance) {
                best_cluster = j;
                best_distance = d;
            }
        }
        cluster_assignment[i] = best_cluster;
        cluster_size[best_cluster]++;
        cluster_sum[best_cluster] += minor_coordinate[i];
    }
   
    // Perform an iterative process until the allocation of clusters no longer changes
    for (int k = 0; k < iter; k++){

        // Update the centre position of each cluster
        std::vector<int> new_centers(num_clusters);
        for (int i = 0; i < num_clusters; i++) {
            std::cout<<"cluster size "<<cluster_size[i]<<"\n";
            if (cluster_size[i] > 0) {
                cluster_center[i] = (float)cluster_sum[i] / cluster_size[i];
                new_centers[i] = cluster_center[i];
                
            }
            else{
                // If there are more num_clusters, then add a random cluster centre
                int random_index = rand() % minor_coordinate.size();
                new_centers[i] = minor_coordinate[random_index];
                cluster_center[i] = (float)new_centers[i];
            }
        }
        chosen_centers = new_centers;

        // Reallocation of clusters per point
        for (int i = 0; i < minor_coordinate.size(); i++) {
            float best_cluster = 0;
            float best_distance = abs(minor_coordinate[i] - chosen_centers[0]);
            for (int j = 1; j < num_clusters; j++) {
                float d = abs(minor_coordinate[j] - chosen_centers[j]);
                if (d < best_distance) {
                    best_cluster = j;
                    best_distance = d;
                }
            }
            if (best_cluster != cluster_assignment[i]) {
                cluster_size[cluster_assignment[i]]--;
                cluster_sum[cluster_assignment[i]] -= minor_coordinate[i];
                cluster_assignment[i] = best_cluster;
                cluster_sum[best_cluster] += minor_coordinate[i];
                cluster_size[best_cluster]++;
            }

        }
    }
    std::sort(cluster_center.begin(), cluster_center.end());
    auto last = std::unique(cluster_center.begin(), cluster_center.end());
    cluster_center.erase(last, cluster_center.end());
    return cluster_center;
}

void reassign_height_new(HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,int chosen_top_K, 
        std::vector<int>& rows_12T,std::vector<int>& rows_8T, int density_12T=3)
    {
        std::unordered_map<int, double> count12TByY;
        std::unordered_map<int, double> count8TByY;
        rows_12T.clear();
        rows_8T.clear();
        vector<int>ylist;
        for (const auto& gate : PlaceInfoMap) {
            int y = gate.second.y;
            std::string cell_type = gate.second.cell_type;
            ylist.push_back(y);
            if (cell_type.find("8T") != std::string::npos) {
                if(count8TByY.find(y) != count8TByY.end())
                {
                    count8TByY[y]+=PlaceInfo_macro[cell_type].area;
                }
                else
                {
                    count8TByY[y]=PlaceInfo_macro[cell_type].area;
                }
            }
            else
            {
                if(count12TByY.find(y) != count12TByY.end())
                {
                    count12TByY[y]+=PlaceInfo_macro[cell_type].area;
                }
                else
                {
                    count12TByY[y]=PlaceInfo_macro[cell_type].area;
                }
            }
        }
        std::sort(ylist.begin(),ylist.end());
        std::vector<int>::iterator s=std::unique(ylist.begin(),ylist.end());
        ylist.erase(s,ylist.end());
        std::vector<std::pair<int, float>> rowScorePairs;
        std::unordered_map<int, string>row_height;
        for(int y:ylist)
        {
            rowScorePairs.emplace_back(y,0);
            row_height[y]="8T";
            if(count12TByY.find(y)!=count12TByY.end())
            {
                rowScorePairs.back().second+=count12TByY[y];
            }
            if(count8TByY.find(y)!=count8TByY.end())
            {
                rowScorePairs.back().second-=count8TByY[y];
            }
        }
        std::sort(rowScorePairs.begin(), rowScorePairs.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second > rhs.second;
                });
        
        for(int i=0;i<chosen_top_K;i++)
        {
            row_height[rowScorePairs[i].first]="12T";
        }
        // for(int i=0;i<ylist.size();i++)
        // {
        //     // cout<<"fuck "<<i<<' '<<ylist[i]<<' '<<row_height[ylist[i]]<<endl;
        //     if(row_height[ylist[i]]=="12T")continue;
        //     int cnt0=0;
        //     for(int j=0;j+1<2*density_12T;j++)
        //     {
        //         if(i+j<ylist.size()&&row_height[ylist[i+j]]=="8T")
        //         {
        //             cnt0++;
        //         }
        //         else
        //         {
        //             break;
        //         }
        //     }
        //     // cout<<ylist[i]<<' '<<i<<' '<<cnt0<<' '<<density_12T<<endl;
        //     if(cnt0>density_12T)
        //     {
        //         // cout<<"12T "<<ylist[i+cnt0/2]<<endl;
        //         row_height[ylist[i+cnt0/2]]="12T";
        //         i=i+cnt0/2;
        //     }
        // }
        int sum=0;
        for(int i=0;i+1<ylist.size();i++)
        {
            sum++;
            if(row_height[ylist[i]]!=row_height[ylist[i+1]])
            {
                if(sum%2==0)
                {
                    sum=0;
                    continue;
                }
                else
                {
                    if(row_height[ylist[i]]=="8T")
                    {
                        row_height[ylist[i]]="12T";
                        sum=1;
                    }
                    else
                    {
                        row_height[ylist[i+1]]="12T";
                    }
                }
            }
        }
        for(int i=0;i<ylist.size();i++)
        {
            if(row_height[ylist[i]]=="12T")rows_12T.push_back(ylist[i]);
            else rows_8T.push_back(ylist[i]);
        }
    }

void row_base_solver(std::string design_name, HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Macro>& PlaceInfo_macro,
                    std::vector<int>& reassign_minor_rows,std::vector<int>& reassign_major_rows, std::pair<std::string, 
                    std::string>& target, HashTable<std::string>& gateHeightAssignmentMap, std::string phase,
                    int chosen_top_K, int iter, int density_12T)
{
    std::vector<int> minor_coordinates;
        std::vector<int> row_coordinates;
        std::vector<int> chosen_centers;
        std::map<std::string, int> minor_map;
        std::map<std::string, float> minor_width;
        std::map<std::string, int> major_map;
        std::map<std::string, float> major_width;
        // int chosen_top_K = 12; //5 //3
        // target = score_row_height_baseline(PlaceInfoMap,PlaceInfo_macro, minor_coordinates, minor_width, chosen_centers,chosen_top_K, 
        //                                                     row_coordinates, minor_map, major_map,major_width, phase);
        // float threshold_minor = 1.0f; //0.65  //0.95
        // float threshold_major = 1.0f; //0.95 //1.0
        // // int iter = 15; //6 //10

        HashTable<PlaceInfo::Gate> PlaceInfoMap_modify = PlaceInfoMap;
        // reassign_minor_rows = reassign_height(chosen_centers, minor_coordinates, minor_width, threshold_minor, 
        //                                         threshold_major, iter, row_coordinates, 
        //                                         minor_map,   reassign_major_rows);
        target=make_pair("12T","8T");
        reassign_height_new(PlaceInfoMap,PlaceInfo_macro, chosen_top_K, reassign_minor_rows, reassign_major_rows, density_12T);
        std::cout << "Minor_rows: ";
        for (const auto& center : reassign_minor_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;
        std::cout << "Major_rows: ";
        for (const auto& center : reassign_major_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;

        std::string file_path = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_modified_"+phase+".json";
        PlaceInfo::rewrite_def_json(file_path, PlaceInfoMap_modify);
    return;
    if (phase == "baseline" or phase=="experiment"){
    // if(1){
        // Row-based method
        std::vector<int> minor_coordinates;
        std::vector<int> row_coordinates;
        std::vector<int> chosen_centers;
        std::map<std::string, int> minor_map;
        std::map<std::string, float> minor_width;
        std::map<std::string, int> major_map;
        std::map<std::string, float> major_width;
        // int chosen_top_K = 12; //5 //3
        target = score_row_height_baseline(PlaceInfoMap,PlaceInfo_macro, minor_coordinates, minor_width, chosen_centers,chosen_top_K, 
                                                            row_coordinates, minor_map, major_map,major_width);
        // std::cout << "cout row_coordinates:"<<std::endl;
        // for (const auto& element : row_coordinates) {
        //     std::cout << element << " ";
        // }
        // std::cout << std::endl;
        // std::cout << "cout chosen_centers"<<std::endl;
        // for (const auto& element : chosen_centers) {
        //     std::cout << element << " ";
        // }
        // std::cout << std::endl;


        float threshold_minor = 1.0f; //0.65  //0.95
        float threshold_major = 1.0f; //0.95 //1.0
        // int iter = 15; //6 //10

        HashTable<PlaceInfo::Gate> PlaceInfoMap_modify = PlaceInfoMap;
        reassign_minor_rows = reassign_height(chosen_centers, minor_coordinates, minor_width, threshold_minor, 
                                                threshold_major, iter, row_coordinates, PlaceInfoMap_modify,
                                                minor_map, major_map, major_width, reassign_major_rows);
        std::cout << "Minor_rows: ";
        for (const auto& center : reassign_minor_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;
        std::cout << "Major_rows: ";
        for (const auto& center : reassign_major_rows) {
            std::cout << center << " ";
        }
        std::cout << std::endl;

        std::string file_path = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_modified_"+phase+".json";
        PlaceInfo::rewrite_def_json(file_path, PlaceInfoMap_modify);
    } else{
        HashTable<PlaceInfo::Gate> PlaceInfoMap_modify = PlaceInfoMap;
        target = score_row_height_dp(PlaceInfoMap, PlaceInfo_macro, reassign_minor_rows, reassign_major_rows, gateHeightAssignmentMap, PlaceInfoMap_modify);
        std::string file_path = "../examples/benchmarks/" + design_name + "/" + design_name + "_def_modified_"+phase+".json";
        PlaceInfo::rewrite_def_json(file_path, PlaceInfoMap_modify);
    }
    
}

void abacus_legalize(std::string design_name,std::vector<int> reassign_minor_rows,std::vector<int> reassign_major_rows,std::pair<std::string, std::string>& target, string phase, bool remap_flag){
    Parser parser;
    auto input = parser.parse("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.def",
    "../examples/benchmarks/" + design_name + "/" + design_name + "_lef_cell.json",
    "../examples/benchmarks/" + design_name + "/" + design_name + "_def_modified_"+phase+".json",
    reassign_minor_rows,reassign_major_rows,target);

    Legalizer legalizer(input);
    auto result = legalizer.solve(design_name, phase, remap_flag);
}

void row_base_report_timing(std::string design_name, HashTable<std::string> PlaceInfo_verilog, std::string phase, int remap_flag)
{   

    HashTable<PlaceInfo::Gate> PlaceInfoMap_modify;
    HashTable<PlaceInfo::Net> PlaceInfo_net_modify;
    HashTable<PlaceInfo::Pin> PlaceInfo_pin_modify;
    HashTable<std::string> PlaceInfo_gateToNet_modify;
    read_row_base_files(design_name, PlaceInfoMap_modify, PlaceInfo_net_modify, PlaceInfo_pin_modify, PlaceInfo_gateToNet_modify,phase, remap_flag); 

    std::string lib_file = "../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib";
    std::string sdc_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc"; 
    if (phase == "baseline"){
        std::string verilog_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.v";
        createRCTree* tree = new createRCTree(PlaceInfo_net_modify,PlaceInfo_gateToNet_modify, PlaceInfoMap_modify, PlaceInfo_verilog);
        FluteResult result;
        tree->buildTopo_global(lib_file, verilog_file, sdc_file);
        result = tree->runFlute();
        delete tree;
    }else{
        std::string verilog_file = "../examples/benchmarks/" + design_name + "/" + design_name + "_resynthesis_remap_" + std::to_string(remap_flag) + ".v";
        std::ifstream file(verilog_file);
        std::string verilog_data((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();
        PlaceInfo::HashTable<std::string> PlaceInfo_verilog_modify;
        PlaceInfo::parseVerilogFile(verilog_data, PlaceInfo_verilog_modify);

        createRCTree* tree = new createRCTree(PlaceInfo_net_modify,PlaceInfo_gateToNet_modify, PlaceInfoMap_modify, PlaceInfo_verilog_modify);
        FluteResult result;
        tree->buildTopo_global(lib_file, verilog_file, sdc_file);
        result = tree->runFlute();
        delete tree;
    }
}

void read_row_base_files(const std::string& design_name, HashTable<PlaceInfo::Gate>& PlaceInfoMap, HashTable<PlaceInfo::Net>& PlaceInfo_net,
                         HashTable<PlaceInfo::Pin>& PlaceInfo_pin, HashTable<std::string>& PlaceInfo_gateToNet, std::string phase, int remap_flag)
{
    if (phase == "baseline"){
        // read def file
        std::ifstream f("../examples/benchmarks/" + design_name + "/" + design_name + "_def_baseline.json");
        json def_data = json::parse(f);
        PlaceInfo::construct_placeInfo_map(def_data, &PlaceInfoMap);

        // read net file
        std::ifstream f_net("../examples/benchmarks/" + design_name + "/" + design_name + "_net.json");
        json net_data = json::parse(f_net);
        PlaceInfo::process_net(net_data, &PlaceInfo_net, &PlaceInfo_pin, &PlaceInfo_gateToNet, PlaceInfoMap);
    }else{
        // read def file
        std::ifstream f("../examples/benchmarks/" + design_name + "/" + design_name + "_def_resynthesis_remap" + std::to_string(remap_flag) + ".json");
        json def_data = json::parse(f);
        PlaceInfo::construct_placeInfo_map(def_data, &PlaceInfoMap);

        // read net file
        std::ifstream f_net("../examples/benchmarks/" + design_name + "/" + design_name + "_net_modified.json");
        json net_data = json::parse(f_net);
        PlaceInfo::process_net(net_data, &PlaceInfo_net, &PlaceInfo_pin, &PlaceInfo_gateToNet, PlaceInfoMap);
    }
        

}
