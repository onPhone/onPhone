#!/bin/bash

echo "Enter race (terran, protoss, zerg):"
read race

echo "Enter difficulty (VeryEasy, Easy, Medium, Hard, VeryHard):"
read difficulty

echo "Enter map (1 for CactusValleyLE, 2 for BelShirVestigeLE, 3 for ProximaStationLE):"
read mapChoice

if [ "$mapChoice" = "1" ]; then
    map="CactusValleyLE"
elif [ "$mapChoice" = "2" ]; then
    map="BelShirVestigeLE"
elif [ "$mapChoice" = "3" ]; then
    map="ProximaStationLE"
else
    echo "Invalid map choice. Using CactusValleyLE as default."
    map="CactusValleyLE"
fi

cd build
make
cd bin
./BasicSc2Bot -c -a $race -d $difficulty -m $map.SC2Map
cd ..
cd ..