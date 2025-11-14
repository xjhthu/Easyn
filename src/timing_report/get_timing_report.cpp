#include "flute.h"
#include "ot/headerdef.hpp"
#include "../timing.hpp"
#include <string.h>
#include <iostream>
#include <fstream>

float return_absolute_value(float a){
        if (a<0){
            return -a;
        }
        return a;
    }


int main(int argc, char* argv[])
{

    std::vector<int> a = {1, 2, 3, 4};
   
    #pragma omp parallel for default(shared)
    for(auto e : a){
        std::cout<<e<<"\n";
    }
    std::cout<<"after the parrallel\n";
   
    
    
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
    
    std::ofstream log_file;
    log_file.open("../examples/benchmarks/" + design_name + "/" + design_name + "_slack_report.txt");
    // timer.dump_slack(std::cout);
    for (auto g : timer.gates()){
        // std::cout << g.first<<" ";
        std::vector<std::string> pin_names;
        pin_names = g.second.print_pins();
  
        
        std::string output_pinname; 

        for (const auto& pin_name : pin_names) {
            size_t colon_pos = pin_name.find(':');
            if (colon_pos != std::string::npos) {

                std::string extracted_str = pin_name.substr(colon_pos + 1);
                
                if (extracted_str == "Z" || extracted_str == "ZN") {
                    output_pinname = extracted_str;
                }
            } else {
                std::cout << "No colon found in string: " << pin_name << std::endl;
            }
        }

        float current_slack;
        float tmp;
        float opentimer_delay=*timer.report_slack(g.first+":"+output_pinname, ot::MAX, ot::FALL);
        tmp = return_absolute_value(opentimer_delay);
        if (tmp>10000000){
            // std::cout<<*timer.report_at(g.first+":ZN", ot::MAX, ot::FALL)<<'\n';
            current_slack = *timer.report_slack(g.first+":ZN", ot::MAX, ot::FALL);
        }
        else{
            current_slack = opentimer_delay;
        }
        log_file<<g.first<<" "<<current_slack<<"\n";
        
    }
    std::optional<float> tns  = timer.report_tns();
    std::optional<float> wns  = timer.report_wns();
    std::optional<float> area = timer.report_area();
    std::optional<float> lkp =  timer.report_leakage_power();
    log_file << std::fixed << std::setprecision(4); 
    log_file <<"tns_r "<<*tns <<" wns_r "<<*wns<<" area_r "<<*area<< " leakage_power "<<*lkp<< "\n";
    // std::cout<<"timer->num_nets()"<<timer.num_nets()<<std::endl;
    log_file.close();
}