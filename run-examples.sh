#!/usr/bin/env bash

# Verify compiler exists
if [ ! -x "./build/bin/morninglang" ]; then
    echo "Error: morninglang compiler not found or not executable"
    exit 1
fi

# Find all .morning files in examples directory
find examples -type f -name "*.morning" | while read -r file; do
    echo "Processing file: $file"
    
    # Compilation
    if ./build/bin/morninglang -f "$file" -o binary; then
        echo "Successfully compiled: $file"
        
        # Execute binary
        if [ -x "binary" ]; then
            echo "Executing binary for $file"
            ./binary
            rm -f binary
        else
            echo "Error: Binary not created for $file"
        fi
    else
        echo "Compilation failed: $file"
        # Clean up potential leftover binary
        rm -f binary
    fi
    
    echo "--------------------------------------"
done
