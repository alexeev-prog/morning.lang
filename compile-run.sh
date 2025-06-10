#!/usr/bin/env bash

bash build.sh all
./build/bin/morninglang
<<<<<<< HEAD
<<<<<<< HEAD

clang++ -O3 -Igc -lgc ./out.ll -o ./out.bin

./out.bin
=======
lli ./out.ll
>>>>>>> OOP
=======
# lli ./out.ll
clang++ -O3 -lgc -Igc out.ll -o out.bin
>>>>>>> OOP

echo $?

