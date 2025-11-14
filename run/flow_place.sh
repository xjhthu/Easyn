#!/bin/bash

design=$1

# rm -rf ../examples/benchmarks/${design}/timingReports
# rm -rf ../examples/benchmarks/${design}/innovus*

mkdir  ../examples/benchmarks/${design}
cp bak/bench.mapped.sdc ../examples/benchmarks/${design}/${design}.mapped.sdc

python set_innovus_tcl.py innovus/run_innovus.tcl ../examples/benchmarks/${design}/run_innovus.tcl "${design}"
python set_innovus_tcl.py innovus/mmmc.tcl ../examples/benchmarks/${design}/mmmc.tcl "${design}"

cd ../build/
./gen_verilog -design_name ${design}
mv ${design}.v ../examples/benchmarks/${design}/

cd ../examples/benchmarks/${design}/
innovus -no_gui -common_ui -files run_innovus.tcl

cd /research/d4/gds/ypu/research/multi-row-placement/fzliu/ripple/bin/

./placer -cell_lef /research/d4/gds/ypu/research/multi-row-placement/fzliu/nangate15_modified/back_end/lef/mix_cell.lef -tech_lef /research/d4/gds/ypu/research/multi-row-placement/fzliu/nangate15_modified/back_end/lef/NanGate_15nm_OCL.tech.lef -input_def /research/d4/gds/ypu/research/multi-row-placement/fzliu/multi-row-resynthesis/examples/benchmarks/${design}/${design}.placed.def

cd .
sh file.sh ${design}
mv ${design}* /research/d4/gds/ypu/research/multi-row-placement/fzliu/multi-row-resynthesis/examples/benchmarks/${design}/
cd /research/d4/gds/ypu/research/multi-row-placement/fzliu/multi-row-resynthesis/build/
./timing_report -design_name ${design}

cd  /research/d4/gds/ypu/research/multi-row-placement/fzliu/multi-row-resynthesis/util
python3 visualize_placement.py --def_cell_file ../examples/benchmarks/${design}/${design}_def_cell.json \
         --lef_cell_file ../examples/benchmarks/${design}/${design}_lef_cell.json \
         --core_conf_file ../examples/benchmarks/${design}/${design}_core.json \
         --image_path ../examples/benchmarks/${design}/${design}_baseline.png

# python generate_row_assignment_file.py --def_json_path ../examples/benchmarks/${design}/${design}_def_cell.json --slack_report_path ../examples/benchmarks/${design}/${design}_slack_report.txt


