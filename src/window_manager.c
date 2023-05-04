
#include <stdio.h>

#include <SDL2/SDL.h>

#include "window_manager.h"
#include "chip8.h"

static inline void build_texture(WindowManager *window_manager, Chip8 *cpu);

// returns a bool (0 for fail or 1 for success)
inline uint8_t window_manager_init(WindowManager *self, WindowManagerOptions *options) {
  const uint32_t DEFAULT_INIT_FLAGS = 0;
  const uint32_t DEFAULT_WINDOW_FLAGS = 0;

  if(SDL_Init(DEFAULT_INIT_FLAGS) < 0) goto fail;

  const char *title = options->title == NULL
    ? "DefaultTitle"
    : options->title;

  const uint16_t width  = options->width == 0 
    ? 524
    : options->width;

  const uint16_t height = options->height == 0
    ? 1024
    : options->height;

  const uint32_t flags = options->flags == 0
    ? DEFAULT_WINDOW_FLAGS
    : options->flags;

  self->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
  if(self->window == NULL) goto fail;

  self->renderer = SDL_CreateRenderer(self->window, -1, 0);
  if(self->renderer == NULL) goto fail;

  self->texture = SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
  if(self->texture == NULL) goto fail;

  return 1;
  // return before fail

fail:
  if(self->window != NULL) {
    SDL_DestroyWindow(self->window);
    self->window = NULL;
  }

  if(self->renderer != NULL) {
    SDL_DestroyRenderer(self->renderer);
    self->renderer = NULL;
  }

  if(self->texture != NULL) {
    SDL_DestroyTexture(self->texture);
    self->texture = NULL;
  }

  SDL_Quit();

  return 0;
}

inline int window_manager_deinit(WindowManager *self) {
  SDL_DestroyTexture(self->texture);
  self->texture = NULL;

  SDL_DestroyRenderer(self->renderer);
  self->renderer = NULL;

  SDL_DestroyWindow(self->window);
  self->window = NULL;

  SDL_Quit();

  return EXIT_SUCCESS;
}

void window_manager_run(WindowManager *self, int argc, char *argv[]) {
  Chip8 cpu = {0};
  chip8_init(&cpu);

  if(argc <= 1) {
    eprintf("Error - no ROM provided");
    return;
  }
  const char *file_path = argv[1];

  chip8_load_rom(&cpu, file_path);

  uint8_t keep_alive = TRUE;
  while(keep_alive) {
    chip8_cycle(&cpu);

    eprintf("opcode = 0x%.4x, pc = 0x%.4x\n", cpu.opcode, cpu.pc);
    
    // event polling
    SDL_Event event = {0};
    while(SDL_PollEvent(&event) > 0) {
      switch(event.type) {
      case SDL_QUIT:
        keep_alive = FALSE; 
        break;
      
      // TODO; keypresses

      default: {}
      }
    }

    // rendering
    (void) SDL_RenderClear(self->renderer);

    build_texture(self, &cpu);
    SDL_Rect texture_dest = {.x = 0, .y = 0, .w = 1024, .h = 512};

    (void) SDL_RenderCopy(self->renderer, self->texture, NULL, &texture_dest);
    (void) SDL_RenderPresent(self->renderer);

    // sleep 16 ms
    sleep(16);
  }
}

// file local functions

static inline void build_texture(WindowManager *window_manager, Chip8 *cpu) {
  uint32_t *buffer = NULL;
  uint32_t pitch = 0;
    
  (void) SDL_LockTexture(window_manager->texture, NULL, (void**) &buffer, (int*) &pitch);
    
    for(uint32_t i = 0; i < GRAPHICS_SIZE; ++i)
      buffer[i] = cpu->graphics[i] == 1 ? 0xFFFFFFFF : 0x000000FF;

  (void) pitch;
  (void) SDL_UnlockTexture(window_manager->texture);
}