#!/bin/bash

echo "Enter race (terran, protoss, zerg):"
read race

echo "Enter difficulty (VeryEasy, Easy, Medium, Hard, VeryHard):"
read difficulty

cd build
make
cd bin
./BasicSc2Bot -c -a $race -d $difficulty -m CactusValleyLE.SC2Map
cd ..
cd ..