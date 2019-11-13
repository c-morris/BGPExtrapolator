#!/bin/bash
#
echo Running $0
#
collectors=(328320 19754 41095 36236 40630 42541 36351 5769 29479 46450 40387 34288 53013 267613 51088 45352 3402 37100 5650 32354 38883 328145 23673 7018 52320)
#
for var in ${collectors[@]}
do
    echo Running bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    echo Running bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
    echo Running bgp-extrapolator -i 0 -m 1 -a mrt_small -v $var -r verify_data_$var\_mo -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -m 1 -a mrt_small -v $var -r verify_data_$var\_mo -f verify_ctrl_$var
done
