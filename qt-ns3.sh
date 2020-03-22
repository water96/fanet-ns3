#!/bin/bash

QT_CREATOR="${QT_PATH}/../Tools/QtCreator/bin/qtcreator"

WORK_DIR="$(dirname $0)"
cd "$WORK_DIR"
WORK_DIR="$PWD"

PROJ_FILE="${1}.creator"

if [ ! -f "${PROJ_FILE}"];
	echo "Project file doesn't exists!"
	exit 1
then

cd $NS3DIR
./waf shell && exit 1


"${$QT_CREATOR}" "${PROJ_FILE}" > /dev/null 2>&1 &
exit 0