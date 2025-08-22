#!/usr/bin/env bash
set -e
set -o pipefail
gcc main.c -Werror -Wpedantic -std=c11 -o app
./app
rm app
