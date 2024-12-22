if not exist build mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
msbuild .\OnPhone.sln
cd ..
set /p race="Enter race (terran, protoss, zerg): "
set /p difficulty="Enter difficulty (VeryEasy, Easy, Medium, Hard, VeryHard): "
.\build\bin\OnPhone.exe -c -a %race% -d %difficulty% -m CactusValleyLE.SC2Map