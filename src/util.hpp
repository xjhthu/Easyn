
#ifndef UTIL_H
#define UTIL_H
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <vector>
#include "dp_solver.hpp"
#include <numeric>
#include <iostream>
#include <algorithm>
using namespace std;
// for string delimiter
namespace Util {
    std::vector<std::string> split(std::string s, std::string delimiter);
    void printAll(const vector<std::vector<string> > &allVecs, size_t vecIndex, string strSoFar,vector<std::vector<string> > &permutation_list);
    uint32_t pareto_solution_select(vector<float>& area_list, vector<float>& delay_list );
    uint32_t solution_select_by_delay(vector<float>& area_list, vector<float>& delay_list );
    bool find_DP_gate_sol_from_list(DPSolver::DP_Gate_Sol gate, vector<DPSolver::DP_Gate_Sol> gate_list);
    bool find_substring_from_string(string substr, string str);
    std::vector<int> pareto_solution_select_tmp(vector<float>& area_list, vector<float>& delay_list );
    float return_absolute_value(float a);
    int return_max_value(vector<float>& v);
    int return_min_value(vector<float>& v);
    void rewrite_def(std::string design_name, std::string phase);
    void dp_table_gate_pruning(DPSolver::HashTable<DPSolver::DP_Gate_Sol>& gateMap);
}
#endif
