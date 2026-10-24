# Malloc alphabet
This program does the following:
Allocate a 26 byte array
Write the alphabet to it
Print the array
Free the allocation

## Compile commands
```
einaplc main.einapl -O -o main.o
gcc -c utils.c -o utils.o -fno-pie -no-pie
gcc utils.o main.o -o main
```
