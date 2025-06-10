#!/usr/bin/env bash

bash build.sh all

clang++ -O3 -Igc -lgc ./out.ll -o ./out.bin

echo $?

