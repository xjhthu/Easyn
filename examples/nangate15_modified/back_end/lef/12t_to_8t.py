import os
import re

cell_name = []
fi = open("NanGate_15nm_OCL_12T.macro.lef", "r+")
with open("NanGate_15nm_OCL_8T.macro.lef", 'w') as f:
    for line in fi.readlines():
        line_data = line.split()
        # print(line_data)
        if line_data == []:
            f.write("\n")
        elif line_data[0] in ['MACRO', 'FOREIGN']:
            cell_name.append(line_data[1])
            f.write(line.replace(line_data[1], line_data[1]+'_8T'))
        elif line_data[0] == 'END' and len(line_data) == 2 and line_data[1] in cell_name:
            f.write(line.replace(line_data[1], line_data[1]+'_8T'))
        elif line_data[0] not in ["SIZE","RECT"]:
            f.write(line)
        elif line_data[0] == "SIZE":
            f.write(line.replace("0.768", "0.512"))
        else:
            small_y = str(round(float(line_data[2])*2/3, 4))
            big_y = str(round(float(line_data[4])*2/3, 4))
            f.write(line.replace(line_data[2], small_y).replace(line_data[4], big_y))
            
