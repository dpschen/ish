#!/bin/sh
set -e

# Build the manual regression helper and execute it so we confirm both sys_read
# and sys_readv leave the descriptor positioned at the bytes copied before a
# fault, and that /proc/self/mem is readable without crashing the kernel.
gcc ../manual/partial_read_fault.c -std=c99 -Wall -Wextra -o partial_read_fault
./partial_read_fault
