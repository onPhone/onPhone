@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
msbuild .\OnPhone.sln
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

:: Parameters
set races=terran protoss zerg
set maps=CactusValleyLE BelShirVestigeLE ProximaStationLE
set runs=3

:initialize_counters
    set /a total_by_race_terran=0, total_by_race_protoss=0, total_by_race_zerg=0
    set /a wins_by_race_terran=0, wins_by_race_protoss=0, wins_by_race_zerg=0
    set /a total_by_map_cactus=0, total_by_map_belshir=0, total_by_map_proxima=0
    set /a wins_by_map_cactus=0, wins_by_map_belshir=0, wins_by_map_proxima=0
    set /a total_games=0, wins=0
goto :eof



:: Run combinations
for /l %%N in (1,1,%runs%) do (
    call :initialize_counters
    for %%R in (%races%) do (
        for %%M in (%maps%) do (
            echo Testing: OnPhone vs %%R : VeryHard on %%M >> %output_file%

            :: Run the game and capture output
            echo Running OnPhone vs %%R : VeryHard on %%M
            .\build\bin\OnPhone.exe -c -a %%R -d VeryHard -m %%M.SC2Map > ../../log.txt
            echo Game run complete!

            :: Extract result from game output
            for /f "tokens=*" %%G in ('findstr "Result:" log.txt') do set "result=%%G"
            for /f "tokens=*" %%G in ('findstr "Total game time:" log.txt') do set "length=%%G"
            for /f "tokens=*" %%G in ('findstr "Game ended after:" log.txt') do set "loops=%%G"

            echo !result! >> %output_file%
            echo !length! >> %output_file%
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
    call :print_summary_statistics
    echo Summary statistics updated in %output_file%. Starting next set of runs...
)


:print_summary_statistics
    :: Function to calculate win rate (integer math for percent)
    set scale_factor=100

    :: Append summary statistics
    (
    echo.
    echo Summary Statistics
    echo ==================
    echo Total Games: %total_games%
    echo Total Wins: %wins%

    :: Calculate win percentage
    set /a scaled_wins=%wins% * %scale_factor% / %total_games%
    echo Win Rate: !scaled_wins!.00%%

    echo.
    echo Win Rates by Race:
    echo ==================
    :: Terran win rate
    set /a scaled_wins_terran=%wins_by_race_terran% * %scale_factor% / %total_by_race_terran%
    echo terran: !scaled_wins_terran!.00%%

    :: Protoss win rate
    set /a scaled_wins_protoss=%wins_by_race_protoss% * %scale_factor% / %total_by_race_protoss%
    echo protoss: !scaled_wins_protoss!.00%%

    :: Zerg win rate
    set /a scaled_wins_zerg=%wins_by_race_zerg% * %scale_factor% / %total_by_race_zerg%
    echo zerg: !scaled_wins_zerg!.00%%

    echo.
    echo Win Rates by Map:
    echo ==================
    :: Map-specific win rates
    set /a scaled_wins_cactus=%wins_by_map_cactus% * %scale_factor% / %total_by_map_cactus%
    echo CactusValleyLE: !scaled_wins_cactus!.00%%

    set /a scaled_wins_belshir=%wins_by_map_belshir% * %scale_factor% / %total_by_map_belshir%
    echo BelShirVestigeLE: !scaled_wins_belshir!.00%%

    set /a scaled_wins_proxima=%wins_by_map_proxima% * %scale_factor% / %total_by_map_proxima%
    echo ProximaStationLE: !scaled_wins_proxima!.00%%

    echo ----------------------------------------
    echo.

    ) >> %output_file%
goto :eof