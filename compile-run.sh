#!/usr/bin/env bash

bash build.sh all
./build/bin/morninglang
<<<<<<< HEAD

clang++ -O3 -Igc -lgc ./out.ll -o ./out.bin

./out.bin
=======
lli ./out.ll
>>>>>>> OOP

echo $?

