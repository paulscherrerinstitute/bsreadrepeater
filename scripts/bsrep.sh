#!/bin/bash

if [[ "$0" == /* ]]; then
    PROGRAM_DIR=$(dirname ${0})
else
    PROGRAM_DIR=$(realpath $(pwd)/$(dirname ${0}))
fi

export LD_LIBRARY_PATH="$PROGRAM_DIR"
$PROGRAM_DIR/bsrep "$@"
