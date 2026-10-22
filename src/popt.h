#ifndef POPT_H
#define POPT_H

void popt_nim(char* path);
void NimMain();

void poptimize(char* path) {
  NimMain();
  popt_nim(path);
}

#endif
