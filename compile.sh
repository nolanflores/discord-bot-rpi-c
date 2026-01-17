#!/bin/bash
gcc -Wall -Wextra src/main.c src/https_socket.c src/websocket.c src/cJSON.c -I includes -I ./ -o run -lssl -lcrypto