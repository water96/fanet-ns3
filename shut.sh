#!/bin/bash

s1=$(date +%s)

OUT_DIR="$(pwd)/scrip_sim"

for i in {0..9..1}; do SEEDS="${SEEDS} $i"; done

echo "Do GM model..."
for n in {2..20..2}
do 
	for s in {100..200..20}
	do
		parallel -j4 './build-ad_hoc-Desktop_Qt_5_12_6_GCC_64bit-Release/ad_hoc --nodes=${n} --mobility=GM --speed=${s} --out-dir=${OUT_DIR}/GM/${n}n/${s}v --seed={}' ::: ${SEEDS}
	done

done
echo "Done GM model==========================="


echo "Do RWP model..."
for n in {2..20..2}
do 
	for s in {100..200..20}
	do
		parallel -j4 './build-ad_hoc-Desktop_Qt_5_12_6_GCC_64bit-Release/ad_hoc --nodes=${n} --mobility=RWP --speed=${s} --out-dir=${OUT_DIR}/RWP/${n}n/${s}v --seed={}' ::: ${SEEDS}
	done

done
echo "Done RWP model==========================="

s2=$(date +%s)
echo "========================================="
echo "Start in $s1 seconds"
echo "End in $s2 seconds"

#while ps -p 29937; do sleep 5; done ; shutdown -h