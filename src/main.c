
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "chip8.h"
#include "window_manager.h"

int main(int argc, char *argv[]) {
  srand(time(NULL));

  WindowManagerOptions window_options = {0};
  window_options.title  = "CHIP8";
  window_options.width  = 1024;
  window_options.height = 512;

  WindowManager window_manager = {0};
  if(!window_manager_init(&window_manager, &window_options)) return 1;

  window_manager_run(&window_manager, argc, argv);

  return window_manager_deinit(&window_manager);
}