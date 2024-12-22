#!/bin/bash

echo "Enter race (terran, protoss, zerg):"
read race

echo "Enter difficulty (1=VeryEasy, 2=Easy, 3=Medium, 4=MediumHard, 5=Hard, 6=HardVeryHard, 7=VeryHard, 8=CheatVision, 9=CheatMoney, 10=CheatInsane):"
read difficultyChoice

if [ "$difficultyChoice" = "1" ]; then
    difficulty="VeryEasy"
elif [ "$difficultyChoice" = "2" ]; then
    difficulty="Easy"
elif [ "$difficultyChoice" = "3" ]; then
    difficulty="Medium"
elif [ "$difficultyChoice" = "4" ]; then
    difficulty="MediumHard"
elif [ "$difficultyChoice" = "5" ]; then
    difficulty="Hard"
elif [ "$difficultyChoice" = "6" ]; then
    difficulty="HardVeryHard"
elif [ "$difficultyChoice" = "7" ]; then
    difficulty="VeryHard"
elif [ "$difficultyChoice" = "8" ]; then
    difficulty="CheatVision"
elif [ "$difficultyChoice" = "9" ]; then
    difficulty="CheatMoney"
elif [ "$difficultyChoice" = "10" ]; then
    difficulty="CheatInsane"
else
    echo "Invalid difficulty choice. Using Medium as default."
    difficulty="Medium"
fi

echo "Enter map (1=CactusValleyLE, 2=BelShirVestigeLE, 3=ProximaStationLE):"
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

if [ ! -d "build" ]; then
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ../
    cd ..
fi

cd build && make && cd ..
./build/bin/OnPhone -c -a $race -d $difficulty -m $map.SC2Map