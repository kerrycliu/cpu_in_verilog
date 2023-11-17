#!/bin/bash

# Input file containing binary values
input_file="ldst.dat"

# Output file for reformatted instructions
output_file="ldst_cpp.dat"

# Loop through the input file, grouping every 4 lines and reformatting
while IFS= read -r line0 && IFS= read -r line1 && IFS= read -r line2 && IFS= read -r line3; do
    # Reformat the instruction and append to the output file
    echo "$line3$line2$line1$line0" >> "$output_file"
done < "$input_file"

