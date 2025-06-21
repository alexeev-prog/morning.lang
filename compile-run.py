#!/usr/bin/env python3
import sys
import os


def build():
    os.system('bash build.sh all')

def run_morninglang():
    os.system('./build/bin/morninglang')

def clang_compile():
    os.system('clang++ -O3 ./out.ll -o ./out.bin')

def run():
    os.system("""./out.bin
echo $?""")

build()
run_morninglang()

if len(sys.argv) > 1 and sys.argv[1] == "build":
    clang_compile()
    run()
