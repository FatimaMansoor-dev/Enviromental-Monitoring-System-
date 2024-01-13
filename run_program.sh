#!/bin/bash


# Compile the program
gcc main.c header.c -o main -lcurl -lm -pthread $(pkg-config --cflags --libs gtk+-3.0 cairo) -lcairo -lcjson

# Execute the program
./main

