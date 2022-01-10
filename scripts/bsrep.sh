#!/bin/bash

PROGRAM_DIR=$(realpath $(pwd)/$(dirname ${0}))
export LD_LIBRARY_PATH="$PROGRAM_DIR"
$PROGRAM_DIR/bsrep "$@"
