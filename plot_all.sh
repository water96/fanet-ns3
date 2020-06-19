#!/bin/bash

export PATH=/home/aleksey/MATLAB/R2017a/bin:${PATH}

target_dir=$1
out_dir=$2

M="PPRZ GM RPGM"
R="AODV OLSR GPSR"

for m in $M; do
	for r in $R; do
		matlab -nodesktop -nodisplay -sd "${target_dir}/${m}_${r}_NVM" \
				-r "addpath('/home/aleksey/work/first_ns3/utils');pref='${m}_${r}';run_conn;exit;"

		mkdir "${out_dir}/${m}_${r}"
		cp ${target_dir}/${m}_${r}_NVM/*.png "${out_dir}/${m}_${r}/"
	done
done