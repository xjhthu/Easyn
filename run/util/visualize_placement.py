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
    ax.plot([0, 10],[0, 10])
    for gate in data_def:
        cell_type = data_def[gate]["macroName"]
        face_color = "none"
        edge_color = "blue" if "8T" in cell_type else "red"
        # if gate in critical_gates:
        #     face_color = edge_color
        
        # print(cell_type)
        
        x,y = data_def[gate]["x"],data_def[gate]["y"]
        w,h = data_lef[cell_type]["width"],data_lef[cell_type]["height"]
        ax.add_patch(Rectangle((x,y),w,h, edgecolor=edge_color,facecolor=face_color))
    ax.add_patch(Rectangle((0,0),data_core["die"]["xh"],data_core["die"]["yh"], edgecolor='blue',facecolor='none'))
    plt.savefig("{}.png".format(design_name))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--def_cell_file', type=str, help=' file consists of the placed/fixed positions of the macros/standard cells of the design')
    parser.add_argument('--lef_cell_file', type=str, help=' file consists of the cell/macro information in the lef files')
    parser.add_argument('--core_conf_file', type=str, help='Information about the design core, so far including the dimensionality of the die')
    parser.add_argument('--timing_report_path', type=str, help='timing report path')
    args = parser.parse_args()
    visualize_placement(args.def_cell_file,args.lef_cell_file,args.core_conf_file,args.timing_report_path)