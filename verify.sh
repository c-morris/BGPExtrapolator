#!/bin/bash
#
echo Running $0
#
collectors=(23106 50304 395152 8222 53364 8607 59891 53013 6894 20080 8492 28260 3333 32709 12307 25160 48362 15562 198385 328474 16347 53070 14630 32097 4777 47422 6720 25933 395766 5645 59414 61832 35619 29222 204028 28917 47950 204092 1798 1221 35369 205206 28571 20953 15435 29686 12350 43578 49673)
#
for var in ${collectors[@]}
do
    echo Running bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    echo Running bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
done
