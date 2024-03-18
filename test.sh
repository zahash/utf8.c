#!/bin/bash

set -e

gcc -Wall -Wextra -Werror -o test test.c utf8.c
./test
rm -f test
