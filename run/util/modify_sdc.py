import json
import re

#parameters
input_delay = 0
input_transition = 5
output_delay_min = -9
output_delay_max = 89

    
with open('../examples/benchmarks/c7552/c7552_net.json', 'r') as f:
    data = json.load(f)

x_pins = [pin["name"] for net in data for pin in net["pins"] if pin["type"] == "IOpin" and pin['name'].startswith('x')]

input = ''
input_ = []
for x_pin in x_pins:
    input_ = f'set_input_delay {input_delay} -min -rise [get_ports {x_pin}]' +'\n'+\
             f'set_input_delay {input_delay} -min -fall [get_ports {x_pin}]' + '\n' + \
             f'set_input_delay {input_delay} -max -rise [get_ports {x_pin}]' + '\n' + \
             f'set_input_delay {input_delay} -max -fall [get_ports {x_pin}]' + '\n' + \
             f'set_input_transition {input_transition} -min -rise [get_ports {x_pin}]' +'\n'+\
             f'set_input_transition {input_transition} -min -fall [get_ports {x_pin}]' + '\n' + \
             f'set_input_transition {input_transition} -max -rise [get_ports {x_pin}]' + '\n' + \
             f'set_input_transition {input_transition} -max -fall [get_ports {x_pin}]' + '\n' 
    input += input_ + '\n'

y_pins = [pin["name"] for net in data for pin in net["pins"] if pin["type"] == "IOpin" and pin['name'].startswith('y')]

output = ""
output_ = []
for y_pin in y_pins:
    output_ = f"set_output_delay {output_delay_min} -min -rise [get_ports {y_pin}] -clock ext_clk_virt\n" + \
                f"set_output_delay {output_delay_min} -min -fall [get_ports {y_pin}] -clock ext_clk_virt\n" + \
                f"set_output_delay {output_delay_max} -max -rise [get_ports {y_pin}] -clock ext_clk_virt\n" + \
                f"set_output_delay {output_delay_max} -max -fall [get_ports {y_pin}] -clock ext_clk_virt\n" 
    output += output_

clock = f'create_clock -period 100 -name ext_clk_virt'
new_content = clock +'\n'+ input + "\n"+output
with open('../examples/benchmarks/c7552/c7552.mapped_modify.sdc', 'a') as f:
    f.write(new_content)



    
