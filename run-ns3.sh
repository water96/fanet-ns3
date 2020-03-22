#!/bin/bash

if [ ! -d "$NS3DIR" ]; then
	echo "Ns-3 directory not found!"
	exit 1
fi

CWD="$PWD"
cd "$NS3DIR" >/dev/null
./waf --cwd="$CWD" $*
cd - >/dev/null

exit 0