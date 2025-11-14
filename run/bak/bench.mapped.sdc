set sdc_version 2.0

# Set the current design


create_clock -period 200 -name ext_clk_virt

set_input_delay 0 -min -rise [get_ports x*]
set_input_delay 0 -min -fall [get_ports x*]
set_input_delay 0 -max -rise [get_ports x*]
set_input_delay 0 -max -fall [get_ports x*]
set_input_transition 5 -min -rise [get_ports x*]
set_input_transition 5 -min -fall [get_ports x*]
set_input_transition 5 -max -rise [get_ports x*]
set_input_transition 5 -max -fall [get_ports x*]
set_output_delay -9 -min -rise [get_ports y*] -clock ext_clk_virt
set_output_delay -9 -min -fall [get_ports y*] -clock ext_clk_virt
set_output_delay 89 -max -rise [get_ports y*] -clock ext_clk_virt
set_output_delay 89 -max -fall [get_ports y*] -clock ext_clk_virt

set_load -pin_load 0.1 [get_ports y*]
