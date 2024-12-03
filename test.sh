#!/bin/bash

# Output file for statistics
output_file="test-results.txt"
echo "Bot Test Results" > $output_file
echo "==================" >> $output_file
date >> $output_file
echo "" >> $output_file

# Arrays of test parameters
races=("terran" "protoss" "zerg")
difficulties=("Hard" "VeryHard")
maps=("CactusValleyLE" "BelShirVestigeLE" "ProximaStationLE")

# Initialize counters
declare -i total_by_race_terran=0 total_by_race_protoss=0 total_by_race_zerg=0
declare -i wins_by_race_terran=0 wins_by_race_protoss=0 wins_by_race_zerg=0
declare -i total_by_difficulty_hard=0 total_by_difficulty_veryhard=0
declare -i wins_by_difficulty_hard=0 wins_by_difficulty_veryhard=0
declare -i total_by_map_cactus=0 total_by_map_belshir=0 total_by_map_proxima=0
declare -i wins_by_map_cactus=0 wins_by_map_belshir=0 wins_by_map_proxima=0
total_games=0
wins=0

# Run combinations
for race in "${races[@]}"; do
    for difficulty in "${difficulties[@]}"; do
        for map in "${maps[@]}"; do
            echo "Testing: OnPhone vs $race : $difficulty on $map" | tee -a $output_file

            # Run the game and capture output
            cd build
            make
            cd bin
            game_output=$(timeout 200s ./BasicSc2Bot -c -a "$race" -d "$difficulty" -m "$map.SC2Map")
            cd ../..

            # Extract result from game output
            result=$(echo "$game_output" | grep "Result:")
            time=$(echo "$game_output" | grep "Total game time:")
            loops=$(echo "$game_output" | grep "Game ended after:")
            # Record results
            echo "$result" >> $output_file
            echo "$time" >> $output_file
            echo "----------------------------------------" >> $output_file

            # Update counters
            case $race in
                "terran") ((total_by_race_terran++));;
                "protoss") ((total_by_race_protoss++));;
                "zerg") ((total_by_race_zerg++));;
            esac

            case $difficulty in
                "Hard") ((total_by_difficulty_hard++));;
                "VeryHard") ((total_by_difficulty_veryhard++));;
            esac

            case $map in
                "CactusValleyLE") ((total_by_map_cactus++));;
                "BelShirVestigeLE") ((total_by_map_belshir++));;
                "ProximaStationLE") ((total_by_map_proxima++));;
            esac

            if [[ $result == *"Won"* ]]; then
                case $race in
                    "terran") ((wins_by_race_terran++));;
                    "protoss") ((wins_by_race_protoss++));;
                    "zerg") ((wins_by_race_zerg++));;
                esac

                case $difficulty in
                    "Hard") ((wins_by_difficulty_hard++));;
                    "VeryHard") ((wins_by_difficulty_veryhard++));;
                esac

                case $map in
                    "CactusValleyLE") ((wins_by_map_cactus++));;
                    "BelShirVestigeLE") ((wins_by_map_belshir++));;
                    "ProximaStationLE") ((wins_by_map_proxima++));;
                esac
                ((wins++))
            fi
            ((total_games++))
        done
    done
done

# Calculate and append summary statistics
echo "" >> $output_file
echo "Summary Statistics" >> $output_file
echo "==================" >> $output_file
echo "Total Games: $total_games" >> $output_file
echo "Total Wins: $wins" >> $output_file
win_percentage=$(bc <<< "scale=2; $wins*100/$total_games")
echo "Win Rate: $win_percentage%" >> $output_file

echo -e "\nWin Rates by Race:" >> $output_file
echo "==================" >> $output_file
win_rate_terran=$(bc <<< "scale=2; $wins_by_race_terran*100/$total_by_race_terran")
win_rate_protoss=$(bc <<< "scale=2; $wins_by_race_protoss*100/$total_by_race_protoss")
win_rate_zerg=$(bc <<< "scale=2; $wins_by_race_zerg*100/$total_by_race_zerg")
echo "terran: $win_rate_terran%" >> $output_file
echo "protoss: $win_rate_protoss%" >> $output_file
echo "zerg: $win_rate_zerg%" >> $output_file

echo -e "\nWin Rates by Difficulty:" >> $output_file
echo "==================" >> $output_file
win_rate_hard=$(bc <<< "scale=2; $wins_by_difficulty_hard*100/$total_by_difficulty_hard")
win_rate_veryhard=$(bc <<< "scale=2; $wins_by_difficulty_veryhard*100/$total_by_difficulty_veryhard")
echo "Hard: $win_rate_hard%" >> $output_file
echo "VeryHard: $win_rate_veryhard%" >> $output_file

echo -e "\nWin Rates by Map:" >> $output_file
echo "==================" >> $output_file
win_rate_cactus=$(bc <<< "scale=2; $wins_by_map_cactus*100/$total_by_map_cactus")
win_rate_belshir=$(bc <<< "scale=2; $wins_by_map_belshir*100/$total_by_map_belshir")
win_rate_proxima=$(bc <<< "scale=2; $wins_by_map_proxima*100/$total_by_map_proxima")
echo "CactusValleyLE: $win_rate_cactus%" >> $output_file
echo "BelShirVestigeLE: $win_rate_belshir%" >> $output_file
echo "ProximaStationLE: $win_rate_proxima%" >> $output_file

echo "Testing complete! Results saved to $output_file"