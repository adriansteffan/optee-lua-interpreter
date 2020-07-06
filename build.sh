#!/bin/bash

# This script is used on my personal development machine. If you want to build the app yourself, you will need to use a combination of the commands below.
# I plan to make a more generalized version of this script later on.

# Execute this before running build or add these to your PATH
# export PATH=$PATH:$HOME/Uni/Thesis/optee/toolchains/aarch32/bin:$HOME/Uni/Thesis/optee/toolchains/aarch64/bin

cd host

make VEBOSE=1 \
    CROSS_COMPILE=aarch64-linux-gnu- \
    TEEC_EXPORT=~/Uni/Thesis/optee/optee_client/out/export/usr \
    --no-builtin-variables

cd ../ta

make \
    CROSS_COMPILE=aarch64-linux-gnu- \
    PLATFORM=rpi3 \
    TA_DEV_KIT_DIR=~/Uni/Thesis/optee/optee_os/out/arm/export-ta_arm64
cd ..

python3 encrypt_lua.py ./example_lua_app/ta

sudo cp ./host/invoke_lua_interpreter ../rpi3-optee/home
sudo cp -r ./example_lua_app ../rpi3-optee/home

sudo cp ./ta/*.ta ../rpi3-optee/lib/optee_armtz


