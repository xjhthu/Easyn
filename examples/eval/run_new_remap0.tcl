set design_name adder

set lef_file {../back_end/lef/NanGate_15nm_OCL.tech.lef ../back_end/lef/NanGate_15nm_OCL_12T_modified.macro.lef ../back_end/lef/NanGate_15nm_OCL_8T.macro.lef}

read_mmmc {./mmmc.tcl}

read_physical -lefs ${lef_file}

set init_design_settop 0

read_netlist ./${design_name}_new_remap_2.v

init_design

write_def -netlist ./${design_name}_0.def

exec python def_coord_update.py --def_file ./${design_name}_0.def --def_json_file ./${design_name}_def_new_remap2.json --new_def_file ./${design_name}_0.def

read_def ./${design_name}_0.def

time_design -pre_cts

report_power
