#!/bin/sh

set -e

gcc -o ./out ./main.c

exec ./out "$@"