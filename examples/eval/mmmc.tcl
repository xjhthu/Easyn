# set design_name [lindex $argv 0]
# puts $design_name
# exit

create_constraint_mode -name my_constraint_mode -sdc_files [list ./$design_name.mapped.sdc]

create_library_set -name PVT_0P72V_125C.setup_set -timing [list ../nangate15_modified/front_end/timing_power_noise/NLDM/NanGate_15nm_OCL_typical_conditional_nldm_8T.lib ../front_end/timing_power_noise/NLDM/NanGate_15nm_OCL_typical_conditional_nldm_12T.lib]

create_timing_condition -name PVT_0P72V_125C.setup_cond -library_sets [list PVT_0P72V_125C.setup_set]

create_rc_corner -name PVT_0P72V_125C.setup_rc -temperature 125.0 

create_delay_corner -name PVT_0P72V_125C.setup_delay -timing_condition PVT_0P72V_125C.setup_cond -rc_corner PVT_0P72V_125C.setup_rc

create_analysis_view -name PVT_0P72V_125C.setup_view -delay_corner PVT_0P72V_125C.setup_delay -constraint_mode my_constraint_mode

create_library_set -name PVT_0P88V_0C.hold_set -timing [list ../front_end/timing_power_noise/NLDM/NanGate_15nm_OCL_typical_conditional_nldm_8T.lib ../front_end/timing_power_noise/NLDM/NanGate_15nm_OCL_typical_conditional_nldm_12T.lib]

create_timing_condition -name PVT_0P88V_0C.hold_cond -library_sets [list PVT_0P88V_0C.hold_set]

create_rc_corner -name PVT_0P88V_0C.hold_rc -temperature 0.0 

create_delay_corner -name PVT_0P88V_0C.hold_delay -timing_condition PVT_0P88V_0C.hold_cond -rc_corner PVT_0P88V_0C.hold_rc

create_analysis_view -name PVT_0P88V_0C.hold_view -delay_corner PVT_0P88V_0C.hold_delay -constraint_mode my_constraint_mode

set_analysis_view -setup { PVT_0P72V_125C.setup_view } -hold { PVT_0P88V_0C.hold_view  }
