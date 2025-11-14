import argparse
import re
from random import randint
import json
import parse_timing_report
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

def parse_slack_report(slack_report_path):
    gate_to_slack_map = {}
    with open(slack_report_path)as f:
        lines = f.readlines()
        for i in range(len(lines)):
            if "Instance " in lines[i]:
                gate = lines[i].strip().split(" ")[1]
                # print(gate)
                if gate in gate_to_slack_map:
                    continue
                else:
                    slack = "lines[i+5].strip().split()[6]"
                    j = i+1
                    while(True):
                        if len(lines[j].split())>5:
                            direction = lines[j].split()[2]
                            if direction=="OUT":
                                slack = float(lines[j].strip().split()[6])
                                break
                        j+=1
                    
                    gate_to_slack_map[gate] = float(slack)
                    # print(gate,slack)
    return gate_to_slack_map


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
    

def timing_aware_random_cell_height_assignment(design_name,def_json_path,slack_report_path,threshold, pure_timing, ctr1=0, ctr2=0, ratio=1.5):
    gate_height_map ={}
    row_to_gate_map = {}
    gate_to_slack_map = {}
    with open(slack_report_path) as f:
        lines = f.readlines()
        for l in lines:
            if l.startswith("g") and len(l.split(" "))==2:
                gate_to_slack_map[l.split(" ")[0]] = float(l.split(" ")[1])
    # gate_to_slack_map =  parse_slack_report("../examples/benchmarks/"+design_name+"/"+design_name+".baseline.timing.rpt")
       
    with open(def_json_path) as f:
        data = json.load(f)
        for g in data:
            if data[g]["y"] not in row_to_gate_map:
                row_to_gate_map[data[g]["y"]] = [(g,data[g]["macroName"])]
            else:
                row_to_gate_map[data[g]["y"]].append((g,data[g]["macroName"]))
    total_cell=0
    for row in row_to_gate_map:
        for (g,m) in row_to_gate_map[row]:
            total_cell+=1
            if "8T" in m:
                gate_height_map[g.strip("g")] = "8T"
            if "12T" in m:
                gate_height_map[g.strip("g")] = "12T"
    print("total cell",total_cell)
    total_num_8T = 0
    total_num_12T = 0
    num_8T_row=0
    num_12T_row=0
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
        # print(row,"8T",ratio_8T,num_of_8T,"12T",ratio_12T,num_of_12T)
        assigned_height = "8T" if num_of_8T*ratio>num_of_12T else "12T"
        if (assigned_height=="8T"):
            num_8T_row+=1
        else:
            num_12T_row+=1
        num_reassign_8T = 0
        num_reassign_12T = 0
        for (g,m) in row_to_gate_map[row]:
            # print(g,m)
            gate_height_map[g.strip("g")] = assigned_height
            if (assigned_height=="8T"):
                if "8T" in m and gate_to_slack_map[g]<0:
                    num_reassign_8T +=1
                    # print("gate",g,"should be assigned to 12T")
            if (assigned_height=="12T"):
                if "12T" in m and gate_to_slack_map[g]>0:
                    num_reassign_12T+=1
                     # print("gate",g,"should be assigned to 8T")
    print("8Trow",num_8T_row,"12Trow",num_12T_row)
        # ratio_reassign = (num_reassign_8T+num_reassign_T)/(num_of_8T+num_of_12T)
       
        # print("accout for reassigned gate ",ratio_reassign)
        
        # if (num_of_8T + num_reassign_12T - num_reassign_8T)*1.5 > num_of_12T + num_reassign_8T - num_reassign_12T:
        #     # print("reassigned for row",row)
        #     assigned_height="8T"
        #     num_8T_row+=1
        # else:
        #     assigned_height="12T"
        #     num_12T_row+=1
        #     # if assigned_height=="12T":
        #     #     assigned_height = "8T"
        #     # else:
        #     #     assigned_height="12T"
        # print(row, assigned_height)
        # for (g,_) in row_to_gate_map[row]:
        #     gate_height_map[g.strip("g")] = assigned_height
        # print("")
    print("12T",num_12T_row,"8T",num_8T_row)
    print("threshold",threshold)
    if(pure_timing):
        print("pure_timing")
        for row in row_to_gate_map:
            for (g,m) in row_to_gate_map[row]:
                if "8T" in m:
                    gate_height_map[g.strip("g")] = "8T"
                else:
                    gate_height_map[g.strip("g")] = "12T"
    
    # print("original ratio","8T",total_num_8T/(total_num_8T+total_num_12T),"12T",total_num_12T/(total_num_8T+total_num_12T))
    # # return
    # num_of_8T_assigned = 0
    # num_of_12T_assigned = 0
    # for g in gate_height_map:
    #     if gate_height_map[g]=="8T":
    #         num_of_8T_assigned+=1
    #     else:
    #         num_of_12T_assigned+=1
    # print("assigned ratio","8T",num_of_8T_assigned/(num_of_8T_assigned+num_of_12T_assigned),"12T",num_of_12T_assigned/(num_of_8T_assigned+num_of_12T_assigned))
    # print("")
    # # gate_to_slack_map =  parse_slack_report("../examples/benchmarks/"+design_name+"/"+design_name+".baseline.timing.rpt")
    # count = 0
    # for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=False):
   
    #     print(count,w, gate_to_slack_map[w])
    #     count+=1
    print("original 8T, but bad timing:")
    for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=False):
        
        # break
        if gate_height_map[w.strip("g")] == "8T":
            gate_height_map[w.strip("g")] = "12T"
            ctr1-=1
       
        if ctr1<=0:
            print(ctr1,w,gate_to_slack_map[w])
            break
    print("original 12T, but good timing:")
    for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=True):
        # break
        if w.strip("g") not in gate_height_map:
            continue
        if gate_height_map[w.strip("g")] == "12T":
            gate_height_map[w.strip("g")] = "8T"
            ctr2-=1
       
        if ctr2<=0:
            print(ctr2,w,gate_to_slack_map[w])
            break
    num_8t = 0
    num_12t=0
    for g in gate_height_map:
        if g.strip("g") not in gate_height_map:
            continue
        if gate_height_map[g]=="8T":
            num_8t+=1
        else:
            num_12t+=1
    print("8t:",num_8t/(num_8t+num_12t),"12t:",num_12t/(num_8t+num_12t))
    json_obj = json.dumps(gate_height_map)

    # print(def_json_path.split("/")[-1].split(".")[0])
    file_name = def_json_path.split("/")[-1].split(".")[0].split("_")[0]
    with open("../examples/benchmarks/"+file_name+"/"+file_name+"_gate_height_assignment.json","w") as f:
        f.write(json_obj) 
def specific_row_assignment(design_name,def_json_path,slack_report_path):
    gate_height_map ={}
    row_to_gate_map = {}
    gate_to_slack_map = {}
    # with open(slack_report_path) as f:
    #     lines = f.readlines()
    #     for l in lines:
    #         if l.startswith("g") and len(l.split(" "))==2:
    #             gate_to_slack_map[l.split(" ")[0]] = float(l.split(" ")[1])
    # print(gate_to_slack_map)
    gate_to_slack_map =  parse_slack_report("../examples/benchmarks/"+design_name+"/"+design_name+".baseline.timing.rpt")
   
    with open(def_json_path) as f:
        data = json.load(f)
        for g in data:
            if data[g]["y"] not in row_to_gate_map:
                row_to_gate_map[data[g]["y"]] = [(g,data[g]["macroName"])]
            else:
                row_to_gate_map[data[g]["y"]].append((g,data[g]["macroName"]))
    # print(row_to_gate_map)
    row_index_list = list(row_to_gate_map.keys())
    row_index_list.sort()
    # print(row_index_list)
    minor_index = []
    for i in range(len(row_index_list)):
        print(i)
        if i in minor_index:
            for g in row_to_gate_map[row_index_list[i]]:
                gate_height_map[g[0].strip("g")] = "12T"
        else:
            for g in row_to_gate_map[row_index_list[i]]:
                gate_height_map[g[0].strip("g")] = "12T"
    # count = 0
    # for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=True):
   
    #     print(count,w, gate_to_slack_map[w])
    #     count+=1
    
    ctr = 0
    for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=True):
        ctr +=1
        print(w, gate_to_slack_map[w])
        gate_height_map[w.strip("g")] = "8T"
        if ctr>0:
            break
   
    num_8t = 0
    num_12t=0
    for g in gate_height_map:
        if gate_height_map[g]=="8T":
            num_8t+=1
        else:
            num_12t+=1
    print("8t:",num_8t/(num_8t+num_12t),"12t:",num_12t/(num_8t+num_12t))
    json_obj = json.dumps(gate_height_map)
    # print(def_json_path.split("/")[-1].split(".")[0])
    file_name = def_json_path.split("/")[-1].split(".")[0].split("_")[0]
    with open("../examples/benchmarks/"+file_name+"/"+file_name+"_gate_height_assignment.json","w") as f:
        f.write(json_obj) 

def cell_height_assignment_only_by_timimg(design_name,def_json_path,slack_report_path,threshold):
    gate_height_map = {}
    
    gate_to_slack_map =  parse_slack_report("../examples/benchmarks/"+design_name+"/"+design_name+".resynthesis_0.timing.rpt")
    num_12T = int(len(gate_to_slack_map)*threshold)
    ctr = 0
    for w in sorted(gate_to_slack_map, key=gate_to_slack_map.get, reverse=False):
        if ctr < num_12T:
            gate_height_map[w.strip("g")] = "12T"
            print("12T", gate_to_slack_map[w])
        else:
            gate_height_map[w.strip("g")] = "8T"
            print("8T", gate_to_slack_map[w])
        ctr +=1
    json_obj = json.dumps(gate_height_map)

    # print(def_json_path.split("/")[-1].split(".")[0])
    file_name = def_json_path.split("/")[-1].split(".")[0].split("_")[0]
    with open("../examples/benchmarks/"+file_name+"/"+file_name+"_gate_height_assignment.json","w") as f:
        f.write(json_obj) 
        
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--design_name', type=str, help='design_name')
    parser.add_argument('--def_json_path', type=str, help='def json path')
    parser.add_argument('--slack_report_path', type=str, help='slack_report_path')
    parser.add_argument('--ctr1', type=int, help='ctr1')
    parser.add_argument('--ctr2', type=int, help='ctr2')
    parser.add_argument('--ratio', type=float, default=1.5, help='ratio')
    parser.add_argument('--pure_timing', type=bool, default=False, help='pure_timing')
    args = parser.parse_args()
    
    # for i in range(1,51):
    #     i /= 100
    #     timing_aware_random_cell_height_assignment(args.design_name,args.def_json_path, args.slack_report_path,i)
    # timing_aware_random_cell_height_assignment(args.design_name,args.def_json_path, args.slack_report_path,0.2,True)
    timing_aware_random_cell_height_assignment(args.design_name,args.def_json_path, args.slack_report_path,0.09,args.pure_timing,args.ctr1,args.ctr2,args.ratio)
    # cell_height_assignment_only_by_timimg(args.design_name,args.def_json_path, args.slack_report_path,0.3)
