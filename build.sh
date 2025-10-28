#!/usr/bin/env bash

# TODO: Makefile

mkdir -p build

g++ -o build/main -Og -Wall src/main.cpp ./glad/src/gl.c -I ./glad/include -l glfw -I include -l assimp "$@"
