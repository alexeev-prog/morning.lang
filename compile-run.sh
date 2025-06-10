#!/usr/bin/env bash

bash build.sh all
./build/bin/morninglang

clang++ -O3 -Igc -lgc ./out.ll -o ./out.bin

./out.bin

echo $?

