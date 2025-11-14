import argparse
import json
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from parse_timing_report import parse_timing_report

def visualize_placement(def_cell_file, lef_cell_file, core_conf_file,timing_report_path):
    design_name = def_cell_file.split("_")[0]
    with open(def_cell_file) as f:
        data_def = json.load(f)
    with open(lef_cell_file) as f:
        data_lef = json.load(f)
    with open(core_conf_file) as f:
        data_core = json.load(f)
    # _, critical_gates = parse_timing_report(timing_report_path)
    fig, ax = plt.subplots()
    # ax.plot([0, 10],[0, 10])
    x_values = [data_def[gate]["x"] for gate in data_def]
    y_values = [data_def[gate]["y"] for gate in data_def]
    min_x = min(x_values)
    max_x = max(x_values)
    min_y = min(y_values)
    max_y = max(y_values)

    # for gate in data_def:
    #     cell_type = data_def[gate]["macroName"]
    #     data_lef[cell_type]["height"]=1024 if "8T" in cell_type else 1536
    # height={}
    # for gate in data_def:
    #     cell_type = data_def[gate]["macroName"]
    #     height[data_def[gate]["y"]] = data_lef[cell_type]["height"]
    # y_values.sort()
    # y_values=list(dict.fromkeys(y_values))
    # sum=0
    # y_map={}

    # c=0
    # lst=0
    # mk={}
    # for i in y_values:
    #     sum+=50
    #     if(c%2 == 0):
    #         sum+=512
    #         if(lst!=height[i]):
    #             sum+=1024
    #     y_map[i]=sum
    #     max_y=max(max_y,y_map[i])
    #     print(i, sum)
    #     if(c%2 == 1 and lst!=height[i]):
    #         sum+=lst
    #         height[i]=lst
    #     else:
    #         sum+=height[i]
    #         lst=height[i]
    #     c+=1
        
    ax.set_xlim(min_x, max_x)
    ax.set_ylim(0, max_y+1536)
    print(min_x,min_y, max_x,max_y)

    total_area = 0
    c=0
    lst=-1
    for gate in data_def:
        cell_type = data_def[gate]["macroName"]
        face_color = "white" if "8T" in cell_type else "white"
        edge_color = "blue" if "8T" in cell_type else "red"
        # if(height[data_def[gate]["y"]] != data_lef[cell_type]["height"]):
        #     continue
        # print(edge_color)  
        # if gate in critical_gates:
        #     face_color = edge_color
        
        # print(cell_type)
        # x,y = data_def[gate]["x"],y_map[data_def[gate]["y"]]
        x,y = data_def[gate]["x"],data_def[gate]["y"]
        w,h = data_lef[cell_type]["width"],1024
        total_area += (data_lef[cell_type]["width"]/2000)*(data_lef[cell_type]["height"]/2000)
        ax.add_patch(Rectangle((x,y),w,h, edgecolor=edge_color,facecolor=face_color))
    # ax.add_patch(Rectangle((0,0),data_core["die"]["xh"],data_core["die"]["yh"], edgecolor='green',facecolor='none'))
    plt.savefig(args.image_path)
    print(args.image_path,total_area)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--def_cell_file', type=str, help=' file consists of the placed/fixed positions of the macros/standard cells of the design')
    parser.add_argument('--lef_cell_file', type=str, help=' file consists of the cell/macro information in the lef files')
    parser.add_argument('--core_conf_file', type=str, help='Information about the design core, so far including the dimensionality of the die')
    parser.add_argument('--timing_report_path', type=str, help='timing report path')
    parser.add_argument('--image_path', type=str, help='path of image')
    args = parser.parse_args()
    visualize_placement(args.def_cell_file,args.lef_cell_file,args.core_conf_file,args.timing_report_path)