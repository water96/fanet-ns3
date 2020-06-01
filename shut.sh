#!/bin/bash

s=$(date +%s)

for m in "GM" "RWP"
do
	echo "Model $m:"
	for n in {2..20..2}
	do
		echo "======Node num $n"
		for s in {100..200..20}
		do
			echo "============Speed $s"
			./build-ad_hoc-Desktop_Qt_5_12_6_GCC_64bit-Release/ad_hoc --Nodes=${n} --Mobility=${m} --Tests=30 --Speed=${s}
		done
	done
done

echo "Model CONST"
for n in {2..20..2}
do
	echo "======Node num $n"
	./build-ad_hoc-Desktop_Qt_5_12_6_GCC_64bit-Release/ad_hoc --Nodes=${n} --Mobility=CONST --Tests=30 --Speed=0
done


s1=$(date +%s)
echo "========================================="
echo "Start in $s seconds"
echo "End in $s1 seconds"

#while ps -p 29937; do sleep 5; done ; shutdown -h