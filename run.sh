#!/bin/bash

cd build
make
cd bin
./BasicSc2Bot -c -a terran -d VeryEasy -m CactusValleyLE.SC2Map > ../../log.txt
cd ..
cd ..