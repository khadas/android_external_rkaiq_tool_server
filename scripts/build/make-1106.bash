#!/bin/bash

cmake -D CMAKE_C_COMPILER=/home/lili/hdd/projects/1106-linux/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc \
-D CMAKE_CXX_COMPILER=/home/lili/hdd/projects/1106-linux/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++ \
-D RKAIQ_API_INCLUDE_DIR=/home/lili/hdd/projects/AIQ/rk1109_isp_simulator/rk_aiq/build/linux/output/arm/installed/include \
CMakeLists.txt

