#!/bin/bash
gcc -std=gnu11 -Wall -Wextra src/main.c src/core/*.c src/bot/*.c -I include -I ./ -o run -l ssl -l crypto