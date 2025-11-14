#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include <string.h>
#include <ot/timer/timer.hpp>
#include <nlohmann/json.hpp>

using namespace mockturtle;
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

int main(int argc, char* argv[])
{
    
    std::cout<<"fuck"<<std::endl;
    std::chrono::steady_clock sc;
    // read arguments
    std::string design_name = "tmp";
    int remap_flag = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-design_name")==0){
            ++i;
            design_name = argv[i];
        }
        else if(strcmp(argv[i],"-remap")==0){

            remap_flag = 1;
        }
        else{
            std::cout<<"unkonw parameter "<<argv[i]<<"\n";
            return 1;
        }
       
    }

    ot::Timer timer_before;
    timer_before.read_celllib("../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib")
                .read_verilog("../examples/benchmarks/" + design_name + "/" + design_name + ".v")
                .read_spef("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.spef")
                .read_sdc("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc")
                .update_timing();
    std::cout << "Timing before greedy algorithm: "<< std::endl;
    report_timer(timer_before);

    ot::Timer timer_after;
    timer_after.read_celllib("../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib")
                .read_verilog("../examples/benchmarks/" + design_name + "/" + design_name + "_remap.v")
                .read_spef("../examples/benchmarks/" + design_name + "/" + design_name + ".remapped.spef")
                .read_sdc("../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc")
                .update_timing();
    std::cout << "Timing after dp algorithm: " << std::endl;
    report_timer(timer_after);

    return 0;
}