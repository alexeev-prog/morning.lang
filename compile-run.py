#!/usr/bin/env python3
import os


def build():
    os.system('bash build.sh all')

def run_morninglang():
    os.system('./build/bin/morninglang')

def clang_compile():
    os.system('clang++ -O3 -Igc -lgc ./out.ll -o ./out.bin')

def run():
    os.system("""./out.bin
echo $?""")

build()
run_morninglang()
clang_compile()
run()
