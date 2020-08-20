#!/bin/bash

RUNS="21..1..30"
out="$(echo $RUNS | sed 's|\..|_|g')"
out_dir="./trans_sims"


pprz_cmd='python3 ./exp.py -j4 -b"PPRZ-OLSR,AODV,GPSR-4..4..16-100..20..200-${RUNS}" -d ${out_dir}/PPRZ_${out}_FULL -l ${out_dir}/pprz_full_${out}.log'
gm_cmd='python3 ./exp.py -j4 -b"GM-OLSR,AODV,GPSR-4..4..16-100..20..200-${RUNS}" -d ${out_dir}/GM_${out}_FULL -l ${out_dir}/gm_full_${out}.log'
rpgm_cmd='python3 ./exp.py -j4 -b"RPGM-OLSR,AODV,GPSR-4..4..16-100..20..200-${RUNS}" -d ${out_dir}/RPGM_${out}_FULL -l ${out_dir}/rpgm_full_${out}.log'
const_cmd='python3 ./exp.py -j4 -b"CONST-OLSR,AODV,GPSR-4..4..16-1-${RUNS}" -d ${out_dir}/CONST_${out}_FULL -l ${out_dir}/const_full_${out}.log'

eval $rpgm_cmd && eval $gm_cmd && eval $pprz_cmd  && eval $const_cmd