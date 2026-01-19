#!/bin/bash
gcc -std=gnu11 -Wall -Wextra src/*.c -I includes -I ./ -o run -lssl -lcrypto