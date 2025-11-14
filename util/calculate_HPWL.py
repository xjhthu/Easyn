import argparse
import json
def calculate_hpwl(design_name,flag):
    if (flag=="baseline"):
        def_json_file ="../examples/benchmarks/{}/{}_def_baseline.json".format(design_name, design_name)
        verilog_file= "../examples/benchmarks/{}/{}.v".format(design_name, design_name)
    else:
        def_json_file ="../examples/benchmarks/{}/{}_def_resynthesis_remap{}.json".format(design_name, design_name,flag)
        verilog_file = "../examples/benchmarks/{}/{}_resynthesis_remap_{}.v".format(design_name, design_name,flag)
    
    with open(verilog_file) as f:
        lines = f.readlines()

    print(def_json_file,verilog_file)
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--design_name', type=str, help='design name')
    args = parser.parse_args()
    calculate_hpwl(args.design_name,"0")
