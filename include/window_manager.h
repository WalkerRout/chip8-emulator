#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "common.h"

typedef struct WindowManager {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
} WindowManager;

typedef struct WindowManagerOptions {
  const char *title;
  uint16_t width;
  uint16_t height;
  uint32_t flags;
} WindowManagerOptions;

uint8_t window_manager_init(WindowManager *self, WindowManagerOptions *options);
int window_manager_deinit(WindowManager *self);
void window_manager_run(WindowManager *self, int argc, char *argv[]);

#endif // WINDOW_MANAGER_H