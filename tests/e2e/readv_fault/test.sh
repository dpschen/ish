#!/bin/sh
set -e

gcc readv_fault.c -Wall -Wextra -O2 -o readv_fault
./readv_fault
