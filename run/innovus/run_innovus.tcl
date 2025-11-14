# set design_name [lindex $argv 0]
set lef_file {/research/d4/gds/ypu/research/multi-row-placement/fzliu/nangate15_modified/back_end/lef/NanGate_15nm_OCL.tech.lef /research/d4/gds/ypu/research/multi-row-placement/fzliu/nangate15_modified/back_end/lef/NanGate_15nm_OCL_12T_modified.macro.lef /research/d4/gds/ypu/research/multi-row-placement/fzliu/nangate15_modified/back_end/lef/NanGate_15nm_OCL_8T.macro.lef}

read_mmmc {./mmmc.tcl}

read_physical -lefs ${lef_file}

set init_design_settop 0

read_netlist ./${design_name}.v

init_design
edit_pin -fixed_pin -pin * -hinst top -pattern fill_optimised -layer { MINT4 MINT6 } -side bottom -start {800 0} -end {0 0}
place_design -no_pre_place_opt

time_design -pre_cts -num_path 100000
report_cell_instance_timing [get_cells -hier -hsc /] -late > ./${design_name}.baseline.timing.rpt
write_def -netlist ${design_name}.placed.def
write_parasitics -spef_file ${design_name}.placed.spef
write_netlist ${design_name}.placed.v
write_sdc ${design_name}.placed.sdc
exit
