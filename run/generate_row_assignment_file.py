import argparse
import re
from random import randint
import json

# def random_cell_height_assignment(verilog_path):
#     gate_height_map ={}
#     with open(verilog_path) as f:
#         lines = f.readlines()
#         for l in lines:
#             # print(l)
#             g = re.findall(r"g[0-9]+",l)
#             if len(g)==1:
               
#                 if randint(1,10)<5:
#                     gate_height_map[g[0].strip("g")] = "8T"
#                 else:
#                     gate_height_map[g[0].strip("g")] = "12T"
#     json_obj = json.dumps(gate_height_map)
#     file_name = verilog_path.split("/")[-1].split(".")[0]
#     with open(file_name+"_gate_height_assignment.json","w") as f:
#         f.write(json_obj) 

def random_cell_height_assignment(def_json_path):
    gate_height_map ={}
    row_to_gate_map = {}
    with open(def_json_path) as f:
        data = json.load(f)
        for g in data:
            if data[g]["y"] not in row_to_gate_map:
                row_to_gate_map[data[g]["y"]] = [(g,data[g]["macroName"])]
            else:
                row_to_gate_map[data[g]["y"]].append((g,data[g]["macroName"]))
    for row in row_to_gate_map:
        num_of_8T = 0
        num_of_12T = 0
        for (g,m) in row_to_gate_map[row]:
            if "8T" in m:
                num_of_8T+=1
            if "12T" in m:
                num_of_12T+=1
        print(row,"8T",num_of_8T/(num_of_8T+num_of_12T),"12T",num_of_12T/(num_of_8T+num_of_12T))
        assigned_height = "8T" if num_of_8T>num_of_12T else "12T"
        for (g,_) in row_to_gate_map[row]:
            gate_height_map[g.strip("g")] = assigned_height
    json_obj = json.dumps(gate_height_map)
    print(def_json_path.split("/")[-1].split(".")[0])
    file_name = def_json_path.split("/")[-1].split(".")[0].split("_")[0]
    with open(file_name+"_gate_height_assignment.json","w") as f:
        f.write(json_obj)   

def timing_aware_random_cell_height_assignment(def_json_path,slack_report_path):
    gate_height_map ={}
    row_to_gate_map = {}
    gate_to_slack_map = {}
    with open(slack_report_path) as f:
        lines = f.readlines()
        for l in lines:
            if l.startswith("g") and len(l.split(" "))==2:
                gate_to_slack_map[l.split(" ")[0]] = float(l.split(" ")[1])
    # print(gate_to_slack_map)
   
    with open(def_json_path) as f:
        data = json.load(f)
        for g in data:
            if data[g]["y"] not in row_to_gate_map:
                row_to_gate_map[data[g]["y"]] = [(g,data[g]["macroName"])]
            else:
                row_to_gate_map[data[g]["y"]].append((g,data[g]["macroName"]))
    total_num_8T = 0
    total_num_12T = 0
    for row in row_to_gate_map:
        num_of_8T = 0
        num_of_12T = 0
        for (g,m) in row_to_gate_map[row]:
            if "8T" in m:
                num_of_8T+=1
                total_num_8T+=1
            if "12T" in m:
                num_of_12T+=1
                total_num_12T+=1
        ratio_8T =  num_of_8T/(num_of_8T+num_of_12T)
        ratio_12T = num_of_12T/(num_of_8T+num_of_12T)
        print(row,"8T",ratio_8T,num_of_8T,"12T",ratio_12T,num_of_12T)
        assigned_height = "8T" if num_of_8T>num_of_12T else "12T"
        num_reassign = 0
        for (g,m) in row_to_gate_map[row]:
            if (assigned_height=="8T"):
                if "8T" in m and gate_to_slack_map[g]<0:
                    num_reassign +=1
                    # print("gate",g,"should be assigned to 12T")
            if (assigned_height=="12T"):
                if "12T" in m and gate_to_slack_map[g]>0:
                    num_reassign+=1
                    # print("gate",g,"should be assigned to 8T")
            
        ratio_reassign = num_reassign/(num_of_8T+num_of_12T)
        print("accout for reassigned gate ",ratio_reassign)
        if ratio_8T - ratio_reassign <0.18 and ratio_12T - ratio_reassign<0.18:
            print("reassigned for row",row)
            if assigned_height=="12T":
                assigned_height = "8T"
            else:
                assigned_height="12T"
        for (g,_) in row_to_gate_map[row]:
            gate_height_map[g.strip("g")] = assigned_height
        print("")
    json_obj = json.dumps(gate_height_map)
    print(def_json_path.split("/")[-1].split(".")[0])
    file_name = def_json_path.split("/")[-1].split(".")[0].split("_")[0]
    with open("../examples/benchmarks/"+file_name+"/"+file_name+"_gate_height_assignment.json","w") as f:
        f.write(json_obj) 
    print("original ratio","8T",total_num_8T/(total_num_8T+total_num_12T),"12T",total_num_12T/(total_num_8T+total_num_12T))
    num_of_8T_assigned = 0
    num_of_12T_assigned = 0
    for g in gate_height_map:
        if gate_height_map[g]=="8T":
            num_of_8T_assigned+=1
        else:
            num_of_12T_assigned+=1
    print("assigned ratio","8T",num_of_8T_assigned/(num_of_8T_assigned+num_of_12T_assigned),"12T",num_of_12T_assigned/(num_of_8T_assigned+num_of_12T_assigned))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--def_json_path', type=str, help='def json path')
    parser.add_argument('--slack_report_path', type=str, help='slack_report_path')
    args = parser.parse_args()
    timing_aware_random_cell_height_assignment(args.def_json_path, args.slack_report_path)

