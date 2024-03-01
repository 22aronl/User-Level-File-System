#!/bin/bash

# Loop through 100 times
for ((i=1; i<=100; i++)); do
    # Generate filename
    filename="../../lab2/t${i}.txt"
    # Generate a random number (between 1 and 100) and write it to the file
    echo $((RANDOM % 100 + 1)) > "$filename"
done