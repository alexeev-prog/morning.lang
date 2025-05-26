#!/usr/bin/env bash

bash build.sh all
./build/morninglang
lli ./out.ll

echo $?
