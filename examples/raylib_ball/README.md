# Raylib Ball
Bouncing ball using raylib. Uses some small helper functions in `ray.c` which handles color and window name.

## Compile commands
```
gcc -c ray.c -o ray.o -fno-pie -no-pie
einaplc main.einapl -O -o main.o
gcc ray.o main.o -lraylib -o main
```
