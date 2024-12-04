# onPhone : A SC2 Zerg Bot

An adaptive Zerg bot for StarCraft II that implements dynamic build orders and intelligent unit control.

## Features

- Dynamic scouting and enemy detection
- Adaptive build order system
- Intelligent unit grouping and control
- Automated testing framework

# Developer Install / Compile Instructions

## Requirements

- [CMake](https://cmake.org/download/)
- Starcraft 2 ([Windows](https://starcraft2.com/en-us/)) ([Linux](https://github.com/Blizzard/s2client-proto#linux-packages))
- [Starcraft 2 Map Packs](https://github.com/Blizzard/s2client-proto#map-packs), The maps we will be using are in the `Ladder 2017 Season 1` pack. Read the instructions for how to extract and where to place the maps.

## Windows

Download and install [Visual Studio 2022](https://www.visualstudio.com/downloads/) if you need it.

```shell
# Clone the project
git clone --recursive https://github.com/onPhone/onPhone.git
cd onPhone

# Create build directory.
mkdir build && cd build

# Generate VS solution.
cmake ../ -G "Visual Studio 17 2022"

# Build the project using Visual Studio.
start BasicSc2Bot.sln
```

## Mac

Note: Try opening the SC2 game client before installing. If the game crashes before opening, you may need to change your Share name:

- Open `System Preferences`
- Click on `Sharing`
- In the `Computer Name` textfield, change the default 'Macbook Pro' to a single word name (the exact name shouldn't matter, as long as its not the default name)

To build, you must use the version of clang that comes with MacOS.

```shell
# Clone the project
git clone --recursive https://github.com/onPhone/onPhone.git
cd onPhone

# Create build directory.
mkdir build && cd build

# Set Apple Clang as the default compiler
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++

# Generate a Makefile
# (Use 'cmake -DCMAKE_BUILD_TYPE=Debug ../' if debug info is needed)
cmake -DCMAKE_BUILD_TYPE=Release ../

# Build
make
```

## Linux

The Linux version is headless, meaning that you will not be able to see your bot
First, download the [Linux package](https://github.com/Blizzard/s2client-proto#linux-packages).
Unzip it to your home directory.
The directory should read as `/home/<USER>/StarCraftII/`.

Rename the `Maps` directory to lowercase, and place any downloaded maps inside this directory:

```bash
mv /home/<USER>/StarCraftII/Maps /home/<USER>/StarCraftII/maps
```

Finally, create a directory (note the added space) which contains a file `ExecuteInfo.txt`, which lists the executable directory:

```bash
mkdir "/home/<USER>/StarCraft II"
echo "executable = /home/<USER>/StarCraftII/Versions/Base75689/SC2_x64" > "/home/<USER>/StarCraft II/ExecuteInfo.txt"
```

The `Base75689` will need to match the correct version which matches the version you downloaded. To check, navigate to `/home/<USER>/StarCraftII/Versions/`.

Remember to replace `<USER>` with the name of your user profile.

# Playing against the built-in AI

In addition to competing against other bots using the [Sc2LadderServer](https://github.com/solinas/Sc2LadderServer), this bot can play against the built-in
AI by specifying command line arguments.

You can find the build target under the `bin` directory. For example, in your root directory:

```shell
# Windows
./build/bin/BasicSc2Bot.exe -c -a zerg -d Hard -m CactusValleyLE.SC2Map

# Mac
./build/bin/BasicSc2Bot -c -a zerg -d Hard -m CactusValleyLE.SC2Map
```

will result in the bot playing against the zerg built-in AI on hard difficulty on the map CactusValleyLE.

Or, you can use this shell script to play against the built-in AI:

```bash
# Windows
scripts/run.bat

# Mac
scripts/run.sh
```

# Automated Testing

Run comprehensive tests across multiple game configurations:

```bash
# Windows
scripts/test.bat

# Mac
scripts/test.sh
```

The script will:

- Test against all races (Terran, Protoss, Zerg)
- Run through multiple difficulty levels
- Test on different maps
- Generate detailed statistics in `test-results-<x>.txt`
