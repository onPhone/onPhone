@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
msbuild .\BasicSc2Bot.sln
cd ..

:: Output file for statistics

@REM Find all files that already exist starting with test-result-x.txt and get the highest number and then add 1 for the new file name
set /a file_number=0
for /f "tokens=*" %%G in ('dir /b test-results-*.txt') do (
    set "file_name=%%G"
    set "file_number=!file_name:~13,-4!"
    if !file_number! GTR %file_number% set /a file_number=!file_number!
)
set /a file_number+=1
set output_file=test-results-%file_number%.txt
echo Bot Test Results > %output_file%
echo ================== >> %output_file%
echo %date% %time% >> %output_file%
echo. >> %output_file%

:: Arrays of test parameters
set races=terran protoss zerg
set maps=CactusValleyLE BelShirVestigeLE ProximaStationLE

:: Initialize counters
set /a total_by_race_terran=0, total_by_race_protoss=0, total_by_race_zerg=0
set /a wins_by_race_terran=0, wins_by_race_protoss=0, wins_by_race_zerg=0
set /a total_by_map_cactus=0, total_by_map_belshir=0, total_by_map_proxima=0
set /a wins_by_map_cactus=0, wins_by_map_belshir=0, wins_by_map_proxima=0
set /a total_games=0, wins=0

:: Run combinations
for %%R in (%races%) do (
    for %%M in (%maps%) do (
        echo Testing: OnPhone vs %%R : VeryHard on %%M >> %output_file%

        :: Run the game and capture output
        cd build
        cd bin
        echo "Running game..."
        BasicSc2Bot.exe -c -a %%R -d VeryHard -m %%M.SC2Map > game_output.txt
        echo "Game run complete!"
        cd ..\..

        :: Read game output
        set "game_output="
        for /f "delims=" %%G in (game_output.txt) do (
            set "game_output=!game_output! %%G"
        )

        echo !game_output!

        :: Extract result from game output
        for /f "tokens=*" %%G in ('echo !game_output! ^| findstr "Result:"') do set "result=%%G"
        for /f "tokens=*" %%G in ('echo !game_output! ^| findstr "Total game time:"') do set "length=%%G"
        for /f "tokens=*" %%G in ('echo !game_output! ^| findstr "Game ended after:"') do set "loops=%%G"
        echo !loops! >> %output_file%

        echo !result! >> %output_file%
        if defined length (
            echo !length! >> %output_file%
        ) else (
            echo "Total game time not found" >> %output_file%
        )
        echo ---------------------------------------- >> %output_file%

        :: Update counters
        if "%%R"=="terran" set /a total_by_race_terran+=1
        if "%%R"=="protoss" set /a total_by_race_protoss+=1
        if "%%R"=="zerg" set /a total_by_race_zerg+=1

        if "%%M"=="CactusValleyLE" set /a total_by_map_cactus+=1
        if "%%M"=="BelShirVestigeLE" set /a total_by_map_belshir+=1
        if "%%M"=="ProximaStationLE" set /a total_by_map_proxima+=1

        if "!result!"=="Result: Won" (
            if "%%R"=="terran" set /a wins_by_race_terran+=1
            if "%%R"=="protoss" set /a wins_by_race_protoss+=1
            if "%%R"=="zerg" set /a wins_by_race_zerg+=1


            if "%%M"=="CactusValleyLE" set /a wins_by_map_cactus+=1
            if "%%M"=="BelShirVestigeLE" set /a wins_by_map_belshir+=1
            if "%%M"=="ProximaStationLE" set /a wins_by_map_proxima+=1
            set /a wins+=1
        )
        set /a total_games+=1
    )
)

:: Calculate and append summary statistics
echo. >> %output_file%
echo Summary Statistics >> %output_file%
echo ================== >> %output_file%
echo Total Games: %total_games% >> %output_file%
echo Total Wins: %wins% >> %output_file%
set /a win_percentage=%wins%*100/%total_games%
echo Win Rate: %win_percentage%%% >> %output_file%

:: Display remaining stats as needed
echo Testing complete! Results saved to %output_file%