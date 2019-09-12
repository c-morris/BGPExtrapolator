#!/bin/bash
#
echo Running $0
#
collectors=(18106 1836 52320 3267 59891 207044 37239 23673 16735 57381)
#
for var in ${collectors[@]}
do
    ./bgp-extrapolator -i 0 -a mrt_small_clean_v3 -v $var -r verify_data_$var -f verify_ctrl_$var
done
