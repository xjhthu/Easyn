import argparse
import json
codeToDefOrientMap = {
    0: "N",
    1: "W",
    2: "S",
    3: "E",
    4: "FN",
    5: "FW",
    6: "FS",
    7: "FE"
}
def update_def_coordinate(def_path, def_json_path, new_def_file):
    gate_coord_map = {}
    def_lines = []
    with open(def_json_path) as f:
        data = json.load(f)
        for g in data:
            gate_coord_map[g] = {
                "x":data[g]["x"],
                "y":data[g]["y"],
                "orient":codeToDefOrientMap[data[g]["orient"]]
            }
    # g268 NAND4_X1_8T + PLACED ( 31744 512 ) FS
    with open(def_path) as f:
        def_lines = f.readlines()
        components_index = [i for i in range(len(def_lines)) if "COMPONENTS" in def_lines[i]]
        
        for i in range(components_index[0]+1,components_index[1]):
            if len(def_lines[i].split(" "))==3:
                gate_name = def_lines[i].split(" ")[1]
                # print(gate_name)
                def_lines[i] =  def_lines[i].strip("\n")+" + PLACED ( "+str(gate_coord_map[gate_name]["x"])+ " "+str(gate_coord_map[gate_name]["y"])+" ) "+ gate_coord_map[gate_name]["orient"]+"\n"

    with open(new_def_file,"w") as fw:
        fw.writelines(def_lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    parser.add_argument('--def_file', type=str, help='def file of the design')
    parser.add_argument('--def_json_file', type=str, help='def file of the design')
    parser.add_argument('--new_def_file', type=str, help='new def file of the design')
    args = parser.parse_args()
    update_def_coordinate(args.def_file,args.def_json_file,args.new_def_file)
