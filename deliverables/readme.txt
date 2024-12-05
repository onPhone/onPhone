      onPhone : A SC2 Zerg Bot
========================================

1. Obtain the Code
-------------------
To get the source code for the onPhone Zerg Bot, clone the repository using Git.
This will download the entire project along with its submodules.

Command:
git clone --recursive https://github.com/onPhone/onPhone.git


2. Navigate to the Project Directory
--------------------------------------
After cloning the repository, change into the project directory to access the files and
folders related to the bot.

Command:
cd onPhone


3. Compile the Code
--------------------
Depending on your operating system, follow the appropriate instructions to compile the bot.

 - Windows:
  1. Create a build directory to keep the build files organized:

     mkdir build && cd build

  2. Generate the Visual Studio solution using CMake:

     cmake ../ -G "Visual Studio 17 2022"

  3. Open the generated solution file (BasicSc2Bot.sln) in Visual Studio and build the
     project by selecting "Build" from the menu or pressing Ctrl+Shift+B.

- Mac:
  1. Create a build directory:

     mkdir build && cd build

  2. Set Apple Clang as the default compiler to ensure compatibility:

     export CC=/usr/bin/clang
     export CXX=/usr/bin/clang++

  3. Generate a Makefile for the project:

     cmake -DCMAKE_BUILD_TYPE=Release ../

  4. Compile the project using the make command:

     make

- Linux:
  1. Install required dependencies:
     sudo apt-get update
     sudo apt-get install cmake build-essential

  2. Ensure you have the Linux package downloaded and unzipped to
     /home/<USER>/StarCraftII/. This is necessary for the bot to function correctly.

  3. Rename the Maps directory to lowercase to comply with the bot's requirements:
     mv /home/<USER>/StarCraftII/Maps /home/<USER>/StarCraftII/maps

  4. Create the ExecuteInfo.txt file, which specifies the executable directory:
     mkdir -p "/home/<USER>/StarCraft II"
     echo "executable = /home/<USER>/StarCraftII/Versions/Base75689/SC2_x64" > "/home/<USER>/StarCraft II/ExecuteInfo.txt"

  5. Build the project:
     mkdir build && cd build
     cmake -DCMAKE_BUILD_TYPE=Release ../
     make


4. Run the Bot
---------------
After compiling, you can run the bot against the built-in AI using the following commands:

Windows:
--------
To run the bot against the Zerg built-in AI on hard difficulty on the map CactusValleyLE:
  ./build/bin/BasicSc2Bot.exe -c -a zerg -d Hard -m CactusValleyLE.SC2Map

Mac/Linux:
---------
To run the bot with the same parameters:
  ./build/bin/BasicSc2Bot -c -a zerg -d Hard -m CactusValleyLE.SC2Map


5. Automated Testing
---------------------
You can also run automated tests using our script to evaluate the bot's performance
across various scenarios:

- Windows:
  Execute the testing script:
  scripts/test.bat

- Mac/Linux:
  Execute the testing script:
  scripts/test.sh

(You may need to make the script executable first: chmod +x scripts/test.sh)

The testing script will:
- Test against all races (Terran, Protoss, Zerg)
- Run through multiple difficulty levels
- Test on different maps
- Generate detailed statistics in test-results-<x>.txt files


6. Additional Information
--------------------------
- Ensure that you have all the required dependencies installed, including CMake and the
  necessary StarCraft II packages and maps.
- Refer to the README.md file in the project for more detailed information about features,
  configuration, and usage.

========================================