#!/bin/bash

WORK_DIR="$(dirname $0)"
cd "$WORK_DIR"
WORK_DIR="$PWD"

PROJ_FILE="${1}"

if [ ! -f "${PROJ_FILE}" ]; then
	echo "Project file doesn't exists!"
	exit 1
fi

qtcreator "${PROJ_FILE}" > /dev/null 2>&1 &
exit 0