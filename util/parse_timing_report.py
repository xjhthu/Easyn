import argparse
import re
def parse_timing_report(report_path):
    path_to_gate_map = {}
    critical_gates = []
    with open(report_path) as f:
        lines = f.readlines()
        block_index = [i for i in range(len(lines)) if lines[i].startswith("Path")]
        print(block_index)
        for i in range(len(block_index)-1):
            path_gates_and_pins = []
            # print(lines[block_index[i]].split())
            slack = float(lines[block_index[i]].split()[3].strip("(").strip("'")) 

            scan_start_index = [k for k in range(block_index[i],block_index[i+1],1) if lines[k].startswith("#---")][0]
            for j in range(scan_start_index,block_index[i+1],1):

                gate = re.findall(r"g[0-9]+/[a-zA-Z]+",lines[j])
                if len(gate)>0:
                    gate = gate[0].split("/")[0]
                    path_gates_and_pins.append(gate)
                    if slack < 0 and gate not in critical_gates:
                        critical_gates.append(gate)
                else:
                    pin = re.findall(r"[xy][0-9]+",lines[j])
                    if len(pin) >0:
                        pin = pin[0]
                        if pin not in path_gates_and_pins:
                            path_gates_and_pins.append(pin)
            if len(path_gates_and_pins)>0:
                path_to_gate_map[i+1] = {
                    "slack":slack,
                    "path_gates":path_gates_and_pins
                }
                # path_to_gate_map[i+1]["slack"] = slack
                # path_to_gate_map[i+1]["path_gates"] = path_gates
    return path_gates_and_pins, critical_gates

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Input file of MPlacer.')
    # parser.add_argument('--net_file', type=str, help='netlist file of the design')
    parser.add_argument('--timing_report_path', type=str, help='timing report path')
    args = parser.parse_args()
    parse_timing_report(args.timing_report_path)