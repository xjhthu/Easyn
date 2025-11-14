#!/bin/bash

design="apex1"
remap_flag="0"

cd /research/d4/spc/ypu/research/multi-row-placement/fzliu/files
./main_${remap_flag} -design_name ${design}
mv ${design}_remap.v ../examples/benchmarks/${design}/

cd ../examples/benchmarks/${design}/
innovus -no_gui -common_ui -files def_rewrite.tcl

cd /research/d4/spc/ypu/research/multi-row-placement/fzliu/files/
./timing_after_remap -design_name ${design}


