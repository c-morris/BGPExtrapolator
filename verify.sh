#!/bin/bash
#
echo Running $0
#
collectors=(24961 23673 29222 25160 64475 15605 56730 61597 1798 9304 29140 49697 30132 49673 6720 35369 8492 2895 37239 39533 42541 25220 50629 327960 5769)
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
