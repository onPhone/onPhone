if not exist build mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
msbuild .\BasicSc2Bot.sln
cd bin
set /p race="Enter race (terran, protoss, zerg): "
set /p difficulty="Enter difficulty (VeryEasy, Easy, Medium, Hard, VeryHard): "
.\BasicSc2Bot.exe -c -a %race% -d %difficulty% -m CactusValleyLE.SC2Map
cd ../..