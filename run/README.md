# multi-row-resynthesis

## Introduction to generating documents in `examples/benchmarks/${design}`

- Innovus input: `c7552.v`, `c7552.mapped.sdc`
- Innovus output: `c7552.def`, `c7552.postplace.sdc`, `c7552.postplace.v`, `c7552.placed.spef`
- ripple output: `c7552_core.json`, `c7552_def_cell.json`, `c7552_lef_cell.json`, `c7552_net.json`
- other files: `c7552.png`

Tips:
- You can use `c7552.postplace.sdc` and `c7552.postplace.v` for timing evaluation, as OpenTimer does not accept the abbreviation IOpin.
- The `c7552.postplace.sdc` file contains the basic instructions that enable timing analysis by OpenTimer. If you only have the `c7552.mapped.sdc` file, you can go to the `util/modify_sdc.py` file to convert the format.
- Commands in the sdc file such as `set_load -pin_load 4 [get_ports x0]` have an effect on both timing result and the spef file. So if you need to change it, please do it before innovus runs.

## Command execution

### Before Started
- bash
- source /opt/rh/devtoolset-8/enable
- cd multi-row-resynthesis
- mkdir build; cd build
- cmake ..
- make -j12 gen_verilog
- make -j12 timing_report

### Get Started
- cd run
- sh flow_place.sh ${design_name}