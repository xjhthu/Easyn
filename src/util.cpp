#include "util.hpp"
#include <cmath>
#include <cstdint>
#include <numeric>
#include <algorithm>
#include <string>
#include <vector>
#include "dp_solver.hpp"
#include "pareto.h"
using namespace std;
namespace Util{
    std::vector<std::string> split(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            // std::cout <<"look "<<token<<" length: "<<token.length()<<"\n";
            if (token.length()>0){
                res.push_back (token);
            }
            
        }

        res.push_back (s.substr (pos_start));
        return res;
    }
    void printAll(const vector<std::vector<string> > &allVecs, size_t vecIndex, string strSoFar,vector<std::vector<string> > &permutation_list)
    {
        if (vecIndex >= allVecs.size())
        {
            
            std::vector<string> permutation_result = split(strSoFar,",");
            permutation_list.push_back(permutation_result);
            return;
        }

        for (size_t i = 0; i < allVecs[vecIndex].size(); i++)
        {
            if( i < allVecs[vecIndex].size() )
            {
                printAll(allVecs, vecIndex + 1, strSoFar + "," + allVecs[vecIndex][i],permutation_list);
            }
        }
    }

    // uint32_t pareto_solution_select(vector<float>& area_list, vector<float>& delay_list ){
    //     uint32_t best_index=0;
    //     float best_area = MAXFLOAT;
    //     auto average_delay = std::accumulate(delay_list.begin(),delay_list.end(),0.0)/delay_list.size();
    //     // std::cout<<"arevage delay "<<average_delay<<"\n";
    //     //TODO: implementation of Pareto point selection
    //     for (int i=0;i<delay_list.size();i++){
    //         // std::cout<<"index "<<i<<" area "<<area_list.at(i)<<" delay "<<delay_list[i]<<"\n";
    //         if(area_list[i]<best_area && delay_list[i]<=average_delay){
    //             best_index = i;
    //             best_area = area_list[i];
    //         }
    //     }
    //     return best_index;
    // }

    uint32_t pareto_solution_select(vector<float>& area_list, vector<float>& delay_list ){
        uint32_t best_index=0;
        float best_area = MAXFLOAT;
        // auto average_delay = std::accumulate(delay_list.begin(),delay_list.end(),0.0)/delay_list.size();
        // std::cout<<"arevage delay "<<average_delay<<"\n";
        //TODO: implementation of Pareto point selection
        for (int i=0;i<delay_list.size();i++){
            // std::cout<<"index "<<i<<" area "<<area_list.at(i)<<" delay "<<delay_list[i]<<"\n";
            if(area_list[i]<best_area){
                best_index = i;
                best_area = area_list[i];
            }
        }
        return best_index;
    }

    std::vector<int> pareto_solution_select_tmp(vector<float>& area_list, vector<float>& delay_list ){
        // vector<int> tmp_list;
        // if (area_list.size()>3){
        //     return {0,1,2};
        // }
        // else{
            
        //     for (int i=0;i<area_list.size();i++){
        //         tmp_list.push_back(i);
        //     }
        //     return tmp_list;
        // }
        Pareto<float, int> pareto;
        assert(area_list.size()==delay_list.size());
        for (int i=0;i<area_list.size();i++){
            vector<float> x;
            x.push_back(-area_list[i]);
            x.push_back(-delay_list[i]);
            pareto.addvalue ( x, i );
        }
        auto pareto_indices = pareto.get_pareto_optimal_indices();
        // std::cout<<"size of pareto points "<<pareto_indices.size()<<"\n";
        return pareto_indices;
    }

    uint32_t solution_select_by_delay(vector<float>& area_list, vector<float>& delay_list ){
        uint32_t best_index=0;
        float best_delay = MAXFLOAT;
        for (int i=0;i<delay_list.size();i++){
            // std::cout<<"index "<<i<<" area "<<area_list.at(i)<<" delay "<<delay_list[i]<<"\n";
            if(delay_list[i]<best_delay){
                best_index = i;
                best_delay = delay_list[i];
            }
        }
        return best_index;
    }

    void dp_table_gate_pruning(DPSolver::HashTable<DPSolver::DP_Gate_Sol>& gateMap){
        vector<float> delay_list;
        vector<float> area_list;
        vector<string>  gate_list;
        for(auto entry : gateMap){
            if((!entry.second.is_calculated)||(entry.second.is_pruned)) return;
        //    std::cout<<"length of delay list "<<entry.second.delay_list.size()<<" is calculated ? "<<entry.second.is_calculated<<"\n";
            gate_list.push_back(entry.first);
            area_list.push_back(entry.second.area);
            delay_list.push_back(std::accumulate(entry.second.delay_list.begin(),entry.second.delay_list.end(),0.0)/entry.second.delay_list.size());
        }
        
        Pareto<float, int> pareto;
        assert(area_list.size()==delay_list.size());
        for (int i=0;i<area_list.size();i++){
            // std::cout<<"area "<<area_list[i]<<" delay "<<delay_list[i]<<"\n";
            vector<float> x;
            x.push_back(-area_list[i]);
            x.push_back(-delay_list[i]);
            pareto.addvalue ( x, i );
        }
        auto pareto_indices = pareto.get_pareto_optimal_indices();
        // std::cout<<"original size "<<gateMap.size()<<" pareto size "<<pareto_indices.size()<<"\n";
        vector<string> maintain_gate_list;
        int iter_size =  (pareto_indices.size()>2)?2:pareto_indices.size();
        for(int i=0;i<iter_size;i++){
            // std::cout<<"pareto index "<<pareto_indices[i]<<"\n";
            maintain_gate_list.push_back(gate_list[pareto_indices[i]]);
        }
    
        for(auto g : gate_list){
            if(!(find(maintain_gate_list.begin(),maintain_gate_list.end(),g)!=maintain_gate_list.end())){
                gateMap[g].is_pruned = true;
            }
        }
        return;
    }

    bool find_DP_gate_sol_from_list(DPSolver::DP_Gate_Sol gate, vector<DPSolver::DP_Gate_Sol> gate_list){
        for (auto g : gate_list){
            if (g.name == gate.name){
                return true;
            }
        }
        return false;
    }
    
    bool find_substring_from_string(string substr, string str){
        if(str.find(substr)!=std::string::npos){
            return true;
        }
        return false;
    }

    float return_absolute_value(float a){
        if (a<0){
            return -a;
        }
        return a;
    }

    int return_max_value(vector<float>& v){
        float max = -1;
        int index = -1;
        for(int i =0;i<v.size();i++){
            if (v[i] >max){
                max = v[i];
                index = i;
            }
        }
        return index;
    }

    int return_min_value(vector<float>& v){
        float min = MAXFLOAT;
        int index = -1;
        for(int i =0;i<v.size();i++){
            if (v[i] <min){
                min = v[i];
                index = i;
            }
        }
        return index;
    }

    void rewrite_def(std::string design_name, std::string phase){
        json def_data;
        
        std::ifstream fin("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.def");
        std::ofstream fout("../examples/benchmarks/" + design_name + "/" + design_name + "_" + phase +".def");
        std::ifstream fdefjson("../examples/benchmarks/" + design_name + "/" + design_name + "_def_" + phase +".json");
        def_data = json::parse(fdefjson);
        size_t rowNum = 0;
        std::string buff;
        while (std::getline(fin, buff))
        {
        

            std::stringstream buffStream(buff);
            std::string identifier, temp;
            buffStream >> identifier;
            fout << buff <<"\n";
            if(identifier=="COMPONENTS"){
                break;
            }
        
        }
        while (std::getline(fin, buff))
        {
        

            std::stringstream buffStream(buff);
            std::string identifier, temp;
            buffStream >> identifier;
            // fout << buff <<"\n";
            std::string gate_name, gate_type, orient;
            int x, y;
            
           
            if(identifier=="-"){
                buffStream >> gate_name >> gate_type >> temp >> temp >> temp >> temp >> temp >>temp>>orient;
                 x = def_data[gate_name]["x"];
                y = def_data[gate_name]["y"];
                fout << "- "<<gate_name <<" "<<gate_type << " + PLACED ( "<<x<<" "<<y<<" ) "<<orient<<"\n";
            }
            else{
                fout<<buff<<"\n";
            }
            if(identifier=="END"){
                break;
            }
        
        }

        while (std::getline(fin, buff))
        {
        

            std::stringstream buffStream(buff);
            std::string identifier, temp;
            buffStream >> identifier;
            fout << buff <<"\n";
           
        }
        fout.close();
    }

    
};
