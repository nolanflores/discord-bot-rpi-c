#!/bin/bash

if [ "$1" = "pi" ]; then
    OPT="-O2"
    FLAGS="-march=native -mtune=cortex-a53"
else
    OPT="-O0"
    if [ "$1" = "debug" ]; then
        FLAGS="-g"
    else
        FLAGS=""
    fi
fi

gcc -std=gnu11 -Wall -Wextra $OPT $FLAGS src/main.c src/core/*.c src/bot/*.c -I include -I ./ -o run -lssl -lcrypto