#!/usr/bin/env bash

syntax-cli -g source/parser/MorningLangGrammar.bnf -m LALR1 -o source/parser/MorningLangGrammar.h
