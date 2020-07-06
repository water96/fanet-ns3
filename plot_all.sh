#!/bin/bash

set -e

target_dir=$1
out_dir=$2

M="PPRZ GM RPGM"
V=(200 200 160)
R="AODV OLSR GPSR"

i=0
for m in $M; do

	matlab -nodesktop -nodisplay -sd "${target_dir}" \
			-r "addpath('/home/aleksey/work/first_ns3/utils');run_conn('${m}');bar_routing_conn('${m}', [1,2,3,4], ${V[$i]});exit;"

	cp $target_dir/MOB_V.png "${out_dir}/${m}_bar_export.png"
	cp $target_dir/mob_conn.png "${out_dir}/${m}_conn_export.png"
	cp $target_dir/${m}_*.png "${out_dir}/"

	for r in $R; do
		if [ ! -d  "${out_dir}/${m}_${r}" ]; then
			mkdir "${out_dir}/${m}_${r}"
		fi
		cp ${target_dir}/${m}_${r}_NVM/*.png "${out_dir}/${m}_${r}/"
	done
	((i=i+1))
done