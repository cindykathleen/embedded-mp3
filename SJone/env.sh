#!/bin/bash
# Setup a base directory:
BASE=/Users/ryanhennings/Developer/SJSU-Dev

# SJSUOne Board Settings:
SJSUONEDEV="/dev/tty.usbserial-AI04D2D0" # Set this to your board ID
SJSUONEBAUD=38400

# Project Target Settings:
# Sets the binary name, defaults to "firmware" (Optional)
PROJ=firmware
# Sets which DBC to generate, defaults to "DBG"
ENTITY=DBG

# Compiler and library settings:
# Selects compiler version to use
PATH=$PATH:$BASE/tools/gcc-arm-none-eabi-6-2017-q2-update/bin:$BASE/tools/Hyperload:$BASE/tools/Telemetry
LIB_DIR="$BASE/firmware/lib"

# Make system settings:
# Number of jobs = 4
# Tune this to nthreads (f.eks. my system has a quad core with two threads per core = 4 threads)
MAKEFLAGS=" -j 4"

# Export everything to the environment
export PATH
export PROJ
export ENTITY
export LIB_DIR
export SJSUONEDEV
export SJSUONEBAUD
export MAKEFLAGS
