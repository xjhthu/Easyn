#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "ot/headerdef.hpp"
#include "../timing.hpp"

int main(int argc, char* argv[]){
   std::string design_name="none";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-design_name")==0){
            ++i;
            design_name = argv[i];
        }
        else{
            std::cout<<"unkonw parameter "<<argv[i]<<"\n";
            return 1;
        }
       
    }
    if(strcmp(design_name.c_str(),"none")==0){
        return 0;
    }
    std::string lib_file = "../examples/benchmarks/NanGate_15nm_OCL_typical_conditional_nldm_8T_12T.lib";
    std::string verilog_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.v";
    std::string spef_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.spef";
    std::string sdc_file = "../examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc";
    // std::string sdc_file = "/research/d4/spc/ypu/research/multi-row-placement/fzliu/examples/benchmarks/" + design_name + "/" + design_name + ".placed.sdc";
    ot::Timer timer;
    // std::cout<<file_sdc<<std::endl;
    timer.read_celllib(lib_file)
             .read_verilog(verilog_file)
             .read_sdc(sdc_file)
             .read_spef(spef_file)
             .update_timing();

    auto start = std::chrono::high_resolution_clock::now();   
    timer.report_at("g130:ZN", ot::MAX,ot::FALL);
    timer.repower_gate("g130","INV_X1_12T");
    timer.report_at("g130:ZN", ot::MAX,ot::FALL);

    timer.report_at("g131:ZN", ot::MAX,ot::FALL);
    timer.repower_gate("g131","INV_X1_12T");
    timer.report_at("g131:ZN", ot::MAX,ot::FALL);

    timer.report_at("g132:ZN", ot::MAX,ot::FALL);
    // timer.repower_gate("g132","INV_X1_12T");
    timer.report_at("g132:ZN", ot::MAX,ot::FALL);
    auto end = std::chrono::high_resolution_clock::now();
    auto time_span = static_cast<std::chrono::duration<double>>(end - start);   // measure time span between start & end
    std::cout<<"DP back-propagation took: "<<time_span.count()<<" seconds !!!\n";
    return 0;
}

