if not exist build mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
msbuild .\BasicSc2Bot.sln
cd bin
.\BasicSc2Bot.exe -c -a terran -d VeryEasy -m CactusValleyLE.SC2Map > ../../log.txt
cd ../..

