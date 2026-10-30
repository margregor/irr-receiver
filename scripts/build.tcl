set proj_name spu
set proj_dir  [pwd]

create_project -name $proj_name -dir $proj_dir/build \
    -pn GW2AR-LV18QN88C8/I7 -device_version C -force

add_file $proj_dir/src/top.sdc
add_file $proj_dir/src/constraints/top.cst
add_file $proj_dir/src/top.sv

set_option -top_module irr_top
set_option -use_cpu_as_gpio 1
set_option -verilog_std sysv2017

run all
