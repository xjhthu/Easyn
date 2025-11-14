#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include "PlaceInfo.hpp"
#include "resynthesis.hpp"
#include <flute.h>
#include <string.h>
#include <string>
#include <ot/timer/timer.hpp>
#include <nlohmann/json.hpp>
#include "timing.hpp"
#include "dp_solver.hpp"
#include "row_base.hpp"
#include "util.hpp"
#include "new_dpsolver.hpp"

using namespace mockturtle;
using namespace PlaceInfo;
using namespace DPSolver;
using json = nlohmann::json;


template<typename T>
using HashTable = std::unordered_map<std::string , T>;
// using cut_t = cut_type<true, empty_cut_data>;
// using cut_set_t = cut_set<cut_t, 26>;
// int resynthesis_single_node(cut_t* cut){
//     // std::cout << "node id "<< node_id <<std::endl;
//     return 0;
// }

void report_timer(ot::Timer& timer)
{
    std::optional<float> tns  = timer.report_tns();
    std::optional<float> wns  = timer.report_wns();
    std::optional<float> area = timer.report_area();
    std::optional<float> lkp =  timer.report_leakage_power();
    std::cout << std::fixed << std::setprecision(4); 
    std::cout <<"tns_r "<<*tns <<" wns_r "<<*wns<<" area_r "<<*area<< " leakage_power "<<*lkp<< "\n";
    std::cout<<"timer->num_nets()"<<timer.num_nets()<<std::endl;
}

void read_placement_info(const std::string& design_name, 
                         PlaceInfo::HashTable<std::string>& PlaceInfo_verilog,
                         PlaceInfo::HashTable<Gate>& PlaceInfoMap, 
                         PlaceInfo::HashTable<Net>& PlaceInfo_net,
                         PlaceInfo::HashTable<Pin>& PlaceInfo_pin, 
                         PlaceInfo::HashTable<std::string>& PlaceInfo_gateToNet,
                         PlaceInfo::HashTable<Macro>& PlaceInfo_macro) {
    // read verilog file
    std::string file_path = "../examples/benchmarks/" + design_name + "/" + design_name + ".v";
    std::ifstream file(file_path);
    std::string verilog_data((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();
    PlaceInfo::parseVerilogFile(verilog_data, PlaceInfo_verilog);

    // read def file
    std::ifstream f("../examples/benchmarks/" + design_name + "/" + design_name + "_def_cell.json");
    json def_data = json::parse(f);
    PlaceInfo::construct_placeInfo_map(def_data, &PlaceInfoMap);

    // read net file
    std::ifstream f_net("../examples/benchmarks/" + design_name + "/" + design_name + "_net.json");
    json net_data = json::parse(f_net);
    PlaceInfo::process_net(net_data, &PlaceInfo_net, &PlaceInfo_pin, &PlaceInfo_gateToNet, PlaceInfoMap);

    // read lef file
    std::ifstream f_lef("../examples/benchmarks/" + design_name + "/" + design_name + "_lef_cell.json");
    json lef_data = json::parse(f_lef);
    PlaceInfo::process_lef(lef_data, &PlaceInfo_macro);
}

int main(int argc, char* argv[])
{
    
    std::cout<<"fuck"<<std::endl;
    std::chrono::steady_clock sc;
    // read arguments
    std::string design_name = "tmp";
    int remap_flag = 0;
    int chosen_top_K = 7;
    int iter = 10;
    int olddp=0;
    double area_slack=0;
    int iteration_rowheight=8,density_12T=3,density_8T=-1;
    double period_extra=400;
    double slack_rate_init=0.01;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-design_name")==0){
            ++i;
            design_name = argv[i];
        }
        else if(strcmp(argv[i],"-remap")==0){

            remap_flag = 1;
        }
        else if(strcmp(argv[i],"-K")==0){
            ++i;
            chosen_top_K = stoi(argv[i]);
        }
        else if(strcmp(argv[i],"-iter")==0){
            ++i;
            iter = stoi(argv[i]);
        }
        else if(strcmp(argv[i],"-olddp")==0){
            olddp=1;
        }
        else if(strcmp(argv[i],"-area_slack")==0){
            ++i;
            area_slack=stod(argv[i]);
        }
        else if(strcmp(argv[i],"-iteration_rowheight")==0){
            ++i;
            iteration_rowheight=stoi(argv[i]);
        }
        else if(strcmp(argv[i],"-density_12T")==0){
            ++i;
            density_12T=stoi(argv[i]);
        }
        else if(strcmp(argv[i],"-density_8T")==0){
            ++i;
            density_8T=stoi(argv[i]);
        }
        else if(strcmp(argv[i],"-period_extra")==0){
            ++i;
            period_extra=stod(argv[i]);
        }
        else if(strcmp(argv[i],"-slack_rate")==0){
            ++i;
            slack_rate_init=stod(argv[i]);
        }
        else{
            std::cout<<"unkonw parameter "<<argv[i]<<"\n";
            return 1;
        }
       
    }

    aig_network aig ;
     auto const result = lorina::read_aiger( "../examples/benchmarks/"+design_name+".aig", mockturtle::aiger_reader( aig ) );
    if ( result != lorina::return_code::success )
    {
        std::cout << "Read benchmark failed\n";
        return -1;
    }
    sequential<klut_network> klut;
   
    std::cout <<"before read gates\n";
    // read library
    std::vector<gate> gates;
    std::ifstream in( "../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.genlib" );
    lorina::read_genlib( in, genlib_reader( gates ) );
    tech_library tech_lib( gates );

    std::cout<<"before mapping\n";
    binding_view<klut_network>  res = mockturtle::map( aig, tech_lib );
    binding_view<klut_network>  res_tmp = mockturtle::map( aig, tech_lib );
    auto const& lib_gates = res.get_library();
    binding_cut_type cuts = fast_cut_enumeration<binding_view<klut_network> ,5, true>( res );
    std::cout<<"after mapping\n";
    
    write_verilog_with_binding(res_tmp, "./"+design_name+".v");
    
    std::ifstream gate_height_f("../examples/benchmarks/"+design_name+"/"+design_name+"_gate_height_assignment.json");
    json gate_height_assignment_data = json::parse(gate_height_f);
    PlaceInfo::HashTable<std::string> gateHeightAssignmentMap;
    PlaceInfo::construct_gate_height_assignment_map(gate_height_assignment_data, &gateHeightAssignmentMap);
  
    // read placement info from def json
    PlaceInfo::HashTable<std::string> PlaceInfo_verilog;
    PlaceInfo::HashTable<Gate> PlaceInfoMap;
    PlaceInfo::HashTable<Net> PlaceInfo_net;
    PlaceInfo::HashTable<Pin> PlaceInfo_pin;
    PlaceInfo::HashTable<std::string> PlaceInfo_gateToNet;
    PlaceInfo::HashTable<Macro> PlaceInfo_macro;
    
    read_placement_info(design_name, PlaceInfo_verilog, PlaceInfoMap, 
                        PlaceInfo_net, PlaceInfo_pin, PlaceInfo_gateToNet, PlaceInfo_macro);    

    // std::vector<int> reassign_minor_rows;
    // std::vector<int> reassign_major_rows;
    // std::pair<std::string, std::string> target;
    // row_base_solver(design_name, PlaceInfoMap,PlaceInfo_macro,reassign_minor_rows,
    //                 reassign_major_rows, target, gateHeightAssignmentMap,"baseline",
    //                 chosen_top_K, iter);
    // std::cout <<"target_minor:"<<target.first<<std::endl;
    // std::cout <<"target_major:"<<target.second<<std::endl;
     
    // abacus_legalize(design_name,reassign_minor_rows,reassign_major_rows,target,"baseline",remap_flag);
    // // row_base_report_timing(design_name, PlaceInfo_verilog,"baseline",remap_flag);
    // // exit(1);
    // Util::rewrite_def(design_name,"baseline");
    // exit(1);
   
    // ot::Timer timer_before;
    // timer_before.read_celllib("../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib")
    //             .read_verilog("../examples/benchmarks/" + design_name + "/" + design_name + ".v")
    //             .read_spef("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.spef")
    //             .read_sdc("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc")
    //             .update_timing();
    // std::cout << "Timing before greedy algorithm: "<< std::endl;
    // report_timer(timer_before);

    // ot::Timer timer_after;
    // timer_after.read_celllib("../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib")
    //             .read_verilog("../examples/benchmarks/" + design_name + "/" + design_name + "_remap.v")
    //             .read_spef("../examples/benchmarks/" + design_name + "/" + design_name + ".remapped.spef")
    //             .read_sdc("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc")
    //             .update_timing();
    // std::cout << "Timing after dp algorithm: " << std::endl;
    // report_timer(timer_after);
    // exit(1);

    std::cout<<"before enter dp solve\n";
    if(olddp==0)
        NewDPSolver::New_dp_solve(&res, lib_gates, gates,
                  cuts, gateHeightAssignmentMap, PlaceInfo_macro, design_name,
                  PlaceInfo_net, PlaceInfo_gateToNet,
                  PlaceInfoMap, PlaceInfo_verilog, remap_flag, chosen_top_K, iter, area_slack, iteration_rowheight, density_12T, density_8T, period_extra, slack_rate_init);
    else
    {auto start_dp = std::chrono::high_resolution_clock::now(); 
    // init_dp_table(&res,lib_gates,gates,cuts);
    dp_solve(&res,lib_gates,gates,cuts,gateHeightAssignmentMap, design_name,PlaceInfo_net,PlaceInfo_gateToNet,PlaceInfoMap, PlaceInfo_verilog,remap_flag);
    auto end_dp = std::chrono::high_resolution_clock::now();       // end timer (starting & ending is done by measuring the time at the moment the process started & ended respectively)
    auto start = std::chrono::high_resolution_clock::now();   
    DPSolver::naive_solution_selection(&res, design_name,remap_flag,PlaceInfoMap);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
    std::cout<<"DP back-propagation took: "<<time_span.count()<<" seconds !!!\n";
    auto time_span_dp = std::chrono::duration_cast<std::chrono::duration<double>>(end_dp - start_dp);   // measure time span between start & end
    std::cout<<"DP took: "<<time_span_dp.count()<<" seconds !!!\n";
    // exit(1);
    std::vector<int> reassign_minor_rows_resynthesis;
    std::vector<int> reassign_major_rows_resynthesis;
    std::pair<std::string, std::string> target_resynthesis;
    row_base_solver(design_name, PlaceInfoMap,PlaceInfo_macro,reassign_minor_rows_resynthesis,
                    reassign_major_rows_resynthesis, target_resynthesis, gateHeightAssignmentMap,"resynthesis",
                    chosen_top_K, iter, density_12T);
    abacus_legalize(design_name,reassign_minor_rows_resynthesis,reassign_major_rows_resynthesis,target_resynthesis,"resynthesis",remap_flag);
    //  row_base_solver(design_name, PlaceInfoMap,PlaceInfo_macro,reassign_minor_rows_resynthesis,
    //                 reassign_major_rows_resynthesis, target_resynthesis, gateHeightAssignmentMap,"experiment",
    //                 chosen_top_K, iter);
    // abacus_legalize(design_name,reassign_minor_rows_resynthesis,reassign_major_rows_resynthesis,target_resynthesis,"experiment",remap_flag);
    
    // rewrite json
    // std::ifstream f_net("../examples/benchmarks/" + design_name + "/" + design_name + "_net.json");
    // json net_data = json::parse(f_net);
    // PlaceInfo::rewrite_net_json(design_name, PlaceInfo_net, net_data);

    // // row_base_report_timing(design_name, PlaceInfo_verilog,"resynthesis",remap_flag);

    // //report_area
    float totalArea = 0.0f;
    // std::unordered_map<std::string, Gate> PlaceInfoMap_resyn;
    // std::ifstream f("../examples/benchmarks/" + design_name + "/" + design_name + "_def_resynthesis_remap" + std::to_string(remap_flag) + ".json");
    // json def_resyn = json::parse(f);
    // PlaceInfo::construct_placeInfo_map(def_resyn, &PlaceInfoMap_resyn);

    for (const auto& entry : PlaceInfoMap) {
        const std::string& cellType = entry.second.cell_type;

        auto macroIt = PlaceInfo_macro.find(cellType);
        if (macroIt != PlaceInfo_macro.end()) {
            const Macro& macro = macroIt->second;
            totalArea += macro.area*10000/2000/2000;
        }
    }

    std::cout << "Total area after resynthesis: " << totalArea << std::endl;
   
    return 0;}
}