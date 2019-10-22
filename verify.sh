#!/bin/bash
#
echo Running $0
#
collectors_old=(23106 50304 395152 8222 53364 8607 59891 53013 6894 20080 8492 28260 3333 32709 12307 25160 48362 15562 198385 328474 16347 53070 14630 32097 4777 47422 6720 25933 395766 5645 59414 61832 35619 29222 204028 28917 47950 204092 1798 1221 35369 205206 28571 20953 15435 29686 12350 43578 49673)
collectors_peer=(51405 35266 18106 29140 28917 8283 25220 50304 53070 196753)
collectors_peer_large=(4739 8283 52320 50877 8896 1798 8455 25160 28220 6894 553 42541 14361 24441 22652 37100 31424 206356 59689 37239 41811 2613 262757 23106 25227)
collectors_cust=(47950 53013 27446 680 45177 12350 7660 198385 27678 1351)
collectors_cust_large=(38883 198385 50673 2895 20932 52320 1836 47422 23106 16347 3402 4739 25160 35266 8607 41722 8492 7660 50304 29686 1280 27678 3292 61832 40191)
#
for var in ${collectors_peer_large[@]}
do
    echo Running bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -a mrt_small -v $var -r verify_data_$var -f verify_ctrl_$var
    echo Running bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -l 1 -a mrt_small -v $var -r verify_data_$var\_oo -f verify_ctrl_$var
    echo Running bgp-extrapolator -i 0 -m 1 -a mrt_small -v $var -r verify_data_$var\_mo -f verify_ctrl_$var
    time ./bgp-extrapolator -i 0 -m 1 -a mrt_small -v $var -r verify_data_$var\_mo -f verify_ctrl_$var
done
