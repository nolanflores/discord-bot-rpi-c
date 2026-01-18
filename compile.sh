#!/bin/bash
gcc -Wall -Wextra src/*.c -I includes -I ./ -o run -lssl -lcrypto