#!/usr/bin/env bash

bash build.sh all
./build/bin/morninglang
# lli ./out.ll
clang++ -O3 -lgc -Igc out.ll -o out.bin

echo $?

# Uncomment if you wanna binary file
# clang out.ll -o out.bin
