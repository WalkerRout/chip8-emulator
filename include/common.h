#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifdef _WINDOWS
  #include <windows.h>
#else
  #include <unistd.h>
  #define Sleep(x) usleep((x)*1000)
#endif
#define sleep Sleep // want to use sleep(...) instead of Sleep(...) to fit conventions

#define TRUE  1
#define FALSE 0

#endif // COMMON_H