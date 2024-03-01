#!/bin/bash

# Define the directory containing the files
DIRECTORY="../ulfs/mount_point/"

# Start timing
start_time=$(date +%s%N)

# Loop through files t1.txt to t100.txt and cat their contents
for ((i=1; i<=100; i++)); do
    FILE="$DIRECTORY""t$i.txt"
    echo "$i"
    if [ -f "$FILE" ]; then
        # Cat the contents of the file
        cat "$FILE"
    else
        # Handle the case where the file is not found
        echo "File $FILE not found"
    fi
done

# End timing
end_time=$(date +%s%N)

# Calculate execution time in milliseconds
execution_time=$(( ($end_time - $start_time) / 1000000 ))

# Print the execution time
echo "Execution time: $execution_time milliseconds"