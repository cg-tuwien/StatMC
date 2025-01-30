#!/bin/bash

# © 2024-2025 Hiroyuki Sakai

# Under Linux, make sure to run with "bash json2cpp.sh" instead of "sh json2cpp.sh" because of the bash-specific array declarations.

PYTHON_BIN="python2.7"
PBRT_BUILD_PATH="../../../../build/pbrt-v3/"

SCRIPT_LOG_PREFIX="[json2cpp.sh]"

if [ "$#" -gt "2" -o "$#" -lt "1" ] ; then
    echo "$SCRIPT_LOG_PREFIX Usage: bash json2cpp.sh /path/to/material.json"
    exit
fi

SCRIPT_PATH="$( cd "$( dirname "$0" )" && pwd )"

echo "$SCRIPT_LOG_PREFIX Started: $( date )"
echo "$SCRIPT_LOG_PREFIX Path: $SCRIPT_PATH"

BASE_FILE_NAME="$( basename $1 .json )"

${PBRT_BUILD_PATH}/albedojson2dat $1 > "./${BASE_FILE_NAME}.dat"

echo "$SCRIPT_LOG_PREFIX Formatting numbers..."
${PYTHON_BIN} json2cpp/format_numbers.py "./${BASE_FILE_NAME}.dat" > "./${BASE_FILE_NAME}-formatted.dat"
rm "./${BASE_FILE_NAME}.dat"

echo "$SCRIPT_LOG_PREFIX Converting to CPP array..."
${PYTHON_BIN} json2cpp/dat2cpp.py "./${BASE_FILE_NAME}-formatted.dat" > "./${BASE_FILE_NAME}.cpp"
rm "./${BASE_FILE_NAME}-formatted.dat"

echo "$SCRIPT_LOG_PREFIX Finished: $( date )"
