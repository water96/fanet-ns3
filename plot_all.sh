#!/bin/bash

export PATH=/home/aleksey/MATLAB/R2017a/bin:${PATH}
set -e

target_dir=$1
out_dir=$2

M="PPRZ GM RPGM"
R="AODV OLSR GPSR"

for m in $M; do
	for r in $R; do
		matlab -nodesktop -nodisplay -sd "${target_dir}/${m}_${r}_NVM" \
				-r "addpath('/home/aleksey/work/first_ns3/utils');pref='${m}_${r}';run_conn;exit;"

		if [ ! -d  "${out_dir}/${m}_${r}" ]; then
			mkdir "${out_dir}/${m}_${r}"
		fi
		cp ${target_dir}/${m}_${r}_NVM/*.png "${out_dir}/${m}_${r}/"
	done


	matlab -nodesktop -nodisplay -sd "${target_dir}" \
			-r "addpath('/home/aleksey/work/first_ns3/utils');bar_routing_conn('${m}', [1,2,3,4], 200);exit;"

	cp $target_dir/MOB_V.png "${out_dir}/${m}_V.png"
done