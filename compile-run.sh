#!/usr/bin/env bash

bash build.sh all
./build/morninglang
lli ./out.ll

echo $?

# Uncomment if you wanna binary file
clang out.ll -o out.bin
