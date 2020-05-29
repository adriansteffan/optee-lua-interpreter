#!/bin/bash

# TODO fix this file

# Execute this before running build or add these to your PATH
# export PATH=$PATH:$HOME/Uni/Thesis/optee/toolchains/aarch32/bin:$HOME/Uni/Thesis/optee/toolchains/aarch64/bin

cd host

make clean

cd ../ta

make clean TA_DEV_KIT_DIR=~/Uni/Thesis/optee/optee_os/out/arm/export-ta_arm64

cd ..



