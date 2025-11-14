import os
import re

# fi = open("NanGate_15nm_OCL_typical_conditional_nldm_12T.lib", "r+")
# with open("NanGate_15nm_OCL_typical_conditional_nldm_8T.lib", 'w') as f:
fi = open("NanGate_15nm_OCL_typical_conditional_nldm_12T.lib", "r+")
line_number = 0
timing_line_number = []
power_9_number = []
with open("NanGate_15nm_OCL_typical_conditional_nldm_8T.lib", 'w') as f:
    for line in fi.readlines():
        line_number += 1
        line_data = line.split()
        # print(line_data)
        if line_data == []:
            f.write("\n")
        elif line_data[0] == 'Module':
            f.write(line.replace(line_data[2], line_data[2]+'_8T'))
        elif line_data[0] == 'cell':
            cell_8t = '(' + line_data[1].replace('(', '').replace(')', '') + '_8T)'
            f.write(line.replace(line_data[1], cell_8t))
        elif line_data[0] in ['area', 'value', 'cell_leakage_power' ,'capacitance', 'fall_capacitance', 'rise_capacitance']:
            small_value = str(round(float(line_data[2].replace(';', ''))*2/3, 6)) + ';'
            f.write(line.replace(line_data[2], small_value))
        elif line_data[0] in ['fall_capacitance_range', 'rise_capacitance_range']:
            small_range = '(' + str(round(float(line_data[1].replace('(', '').replace(',', ''))*2/3, 6)) + ','
            large_range = str(round(float(line_data[2].replace(');', ''))*2/3, 6)) + ');'
            f.write(line.replace(line_data[1], small_range).replace(line_data[2], large_range))
        elif line_data[0] == 'values' and line_number not in timing_line_number and line_number not in power_9_number:
            power_value = re.split(',|"', line_data[1])
            power_str = '("'
            for i in range(1,len(power_value)-1):
                power_str += (str(round(float(power_value[i])*2/3, 6))+',')
            # power_str.strip(',')
            # print('power_str',power_str)
            power_s = power_str[:-1]
            power_s += '");'
            f.write(line.replace(line_data[1], power_s))
        elif 'fall_constraint' in line_data[0] or 'rise_constraint' in line_data[0] or 'cell_fall' in line_data[0] or 'cell_rise' in line_data[0] or 'fall_transition' in line_data[0] or 'rise_transition' in line_data[0]:
        # ['fall_constraint(Hold_5_5)', 'rise_constraint(Hold_5_5)', 'fall_constraint(Setup_5_5)', 'rise_constraint(Setup_5_5)', \
        #  'cell_fall(Timing_9_9)', 'cell_rise(Timing_9_9)', 'fall_transition(Timing_9_9)', 'rise_transition(Timing_9_9)', \
        #  'rise_constraint(Recovery_5_5)', 'rise_constraint(Removal_5_5)', 'fall_constraint(Pulse_width_1)']:
            if '_5_5' in line_data[0]:
                timing_line_number += [i for i in range(line_number+3, line_number+8)]
            elif '_9_9' in line_data[0]:
                timing_line_number += [i for i in range(line_number+3, line_number+12)]
            else:
                if 'scalar' not in line_data[0]:
                    timing_line_number += [line_number+2]
            f.write(line)
        elif line_data[0] in ['fall_power(Power_9_9)', 'rise_power(Power_9_9)']:
            power_9_number += [i for i in range(line_number+3, line_number+12)]
            f.write(line)
        elif line_number in power_9_number:
            if len(line_data) == 3:
                power_value = re.split(',|"', line_data[1])
                power_str = '("'
                for i in range(1,len(power_value)-2):
                    power_str += (str(round(float(power_value[i])*2/3, 6))+',')
                power_str.strip(',')
                power_str += '",'
                f.write(line.replace(line_data[1], power_str))
            elif len(line_data) == 1:
                power_value = re.split(',|"', line_data[0])
                power_str = '"'
                for i in range(1,len(power_value)-1):
                    power_str += (str(round(float(power_value[i])*2/3, 6))+',')
                power_str.strip(',')
                power_str += '");'
                f.write(line.replace(line_data[0], power_str))
            else:
                power_value = re.split(',|"', line_data[0])
                power_str = '"'
                for i in range(1,len(power_value)-2):
                    power_str += (str(round(float(power_value[i])*2/3, 6))+',')
                power_str.strip(',')
                power_str += '",'
                f.write(line.replace(line_data[0], power_str))
        elif line_number in timing_line_number:
            if len(line_data) == 3:
                timing_value = re.split(',|"', line_data[1])
                timing_str = '("'
                for i in range(1,len(timing_value)-2):
                    timing_str += (str(round(float(timing_value[i])*4, 6))+',')
                timing_str.strip(',')
                timing_str += '",'
                f.write(line.replace(line_data[1], timing_str))
            elif len(line_data) == 1:
                timing_value = re.split(',|"', line_data[0])
                timing_str = '"'
                for i in range(1,len(timing_value)-1):
                    timing_str += (str(round(float(timing_value[i])*4, 6))+',')
                timing_str.strip(',')
                timing_str += '");'
                f.write(line.replace(line_data[0], timing_str))
            elif len(line_data) == 2 and line_data[0] == 'values':
                timing_value = re.split(',|"', line_data[1])
                timing_str = '("'
                for i in range(1,len(timing_value)-1):
                    timing_str += (str(round(float(timing_value[i])*4, 6))+',')
                timing_s = timing_str[:-1]
                timing_s += '");'
                f.write(line.replace(line_data[1], timing_s))
            else:
                timing_value = re.split(',|"', line_data[0])
                timing_str = '"'
                for i in range(1,len(timing_value)-2):
                    timing_str += (str(round(float(timing_value[i])*4, 6))+',')
                timing_str.strip(',')
                timing_str += '",'
                f.write(line.replace(line_data[0], timing_str))
        else:
            f.write(line)
        
    # print('timing', timing_line_number)
    # print('power', power_9_number)
            