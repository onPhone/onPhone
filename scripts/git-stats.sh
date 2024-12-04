# To Run, `./git-stats.sh`

# Define color variables
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
RESET='\033[0m'

# Temporary files to store intermediate data
temp_file=$(mktemp)
aggregated_file=$(mktemp)

echo -e "${YELLOW}Commits and Lines of Code per Author (Aggregated and Sorted by Total Changes):${RESET}"
echo "--------------------------------------------------------------------------"

# Define author aliases as a text file (or inline here)
# Format: OriginalName=CanonicalName
aliases=("Nandan=Nandan Ramesh" "Ashwin=Ashwin Shreekumar" "Flaming-Dorito=Ritwik Rastogi" "Patchoulis=Mark Usmanov" "Jake Tuero=Others" "Chris Solinas=Others")

# Function to resolve aliases
resolve_alias() {
    local name="$1"
    for alias in "${aliases[@]}"; do
        original="${alias%%=*}"
        canonical="${alias##*=}"
        if [ "$name" == "$original" ]; then
            echo "$canonical"
            return
        fi
    done
    echo "$name" # Return the original name if no alias matches
}

# Get the list of unique emails from `git log`
for email in $(git log --all --format='%ae' | sort | uniq); do
    # Count commits for the email
    commit_count=$(git log --all --author="$email" --oneline | wc -l)

    # Count lines added and removed
    added=$(git log --all --author="$email" --pretty=tformat: --numstat | awk '{ add += $1 } END { print add }')
    removed=$(git log --all --author="$email" --pretty=tformat: --numstat | awk '{ subs += $2 } END { print subs }')

    # Handle cases where no additions or deletions were found
    added=${added:-0}
    removed=${removed:-0}

    # Calculate total lines changed
    total_lines=$((added + removed))

    # Get the name associated with the email
    author_name=$(git log --all --author="$email" --format='%an' | sort | uniq | head -n 1)

    # Resolve alias if it exists
    resolved_name=$(resolve_alias "$author_name")

    # Output data to temporary file
    echo "$resolved_name|$email|$commit_count|$added|$removed|$total_lines" >> "$temp_file"
done

# Aggregate data for authors with the same resolved name
awk -F'|' '
{
    name = $1
    commits[name] += $3
    added[name] += $4
    removed[name] += $5
    total[name] += $6
}
END {
    for (name in total) {
        printf "%s|%d|%d|%d|%d\n", name, commits[name], added[name], removed[name], total[name]
    }
}' "$temp_file" > "$aggregated_file"

# Sort aggregated data by total lines changed in descending order
sorted_data=$(sort -nr -t '|' -k5 "$aggregated_file")

# Display sorted and aggregated data
echo "$sorted_data" | awk -F'|' -v GREEN="$GREEN" -v BLUE="$BLUE" -v YELLOW="$YELLOW" -v RED="$RED" -v RESET="$RESET" '
{
    name = $1
    commit_count = $2
    added = $3
    removed = $4
    total_lines = $5

    # Print author information
    printf "%s%s%s\n", BLUE, name, RESET
    printf "%sCommits:%s %d\n", GREEN, RESET, commit_count
    printf "%sLines added:%s %d\n", GREEN, RESET, added
    printf "%sLines removed:%s %d\n", RED, RESET, removed
    printf "Total lines changed: %d\n", total_lines
    print "--------------------------------------------------------------------------"
}'

# Clean up
rm "$temp_file" "$aggregated_file"