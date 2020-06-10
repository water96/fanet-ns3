#!/bin/bash
start="$(date)"
while ps -p 1515;
do 
	sleep 30
done
fin="$(date)"
echo "Start at ${start}, finist at ${fin}"
shutdown -h
