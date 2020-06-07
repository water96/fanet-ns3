#!/bin/bash
start="$(date)"
while ps -p 8816;
do 
	sleep 30
done
fin="$(date)"
echo "Start at ${start}, finist at ${fin}"
shutdown -h
