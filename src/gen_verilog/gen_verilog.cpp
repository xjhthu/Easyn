#include <iostream>
#include <mockturtle/mockturtle.hpp>
#include <string.h>
#include <nlohmann/json.hpp>


using namespace mockturtle;
using json = nlohmann::json;
using binding_cut_type = mockturtle::fast_network_cuts<binding_view<klut_network>, 5, true, mockturtle::empty_cut_data>;

template<typename T>
using HashTable = std::unordered_map<std::string , T>;
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
   
    return 0;
}