import os
import re

fi = open("NanGate_15nm_OCL_functional_12T.lib", "r+")
with open("NanGate_15nm_OCL_functional_8T.lib", 'w') as f:
    for line in fi.readlines():
        line_data = line.split()
        # print(line_data)
        if line_data == []:
            f.write("\n")
        elif line_data[0] == 'area':
            small_area = str(round(float(line_data[2].replace(';', ''))*2/3, 6)) + ';'
            f.write(line.replace(line_data[2], small_area))
        elif line_data[0] == 'Module':
            f.write(line.replace(line_data[2], line_data[2]+'_8T'))
        elif line_data[0] == 'cell':
            cell_8t = '(' + line_data[1].replace('(', '').replace(')', '') + '_8T)'
            f.write(line.replace(line_data[1], cell_8t))
        else:
            f.write(line)