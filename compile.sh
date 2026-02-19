#!/bin/bash

if [ "$1" = "pi" ]; then
    OPT="-O2"
    ARCH="-march=native -mtune=cortex-a53"
else
    OPT="-O0"
    ARCH=""
fi

gcc -std=gnu11 -Wall -Wextra $OPT $ARCH src/main.c src/core/*.c src/bot/*.c -I include -I ./ -o run -l ssl -l crypto