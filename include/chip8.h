#ifndef CHIP8_H
#define CHIP8_H

#define STACK_SIZE 16
#define MEMORY_SIZE 4096
#define GRAPHICS_SIZE (64 * 32)
#define REGISTER_COUNT 16
#define KEY_COUNT 16

#include "common.h"

typedef struct Chip8 {
  uint16_t opcode;
  
  uint16_t index;
  uint8_t memory[MEMORY_SIZE];
  uint8_t graphics[GRAPHICS_SIZE];

  uint16_t stack[STACK_SIZE]; // contains addresses, must be uint16_t
  uint8_t sp; // could be 4 bits since only 16 stack items haha

  uint8_t registers[REGISTER_COUNT];
  uint16_t pc;

  uint16_t keys[KEY_COUNT]; // which keys are currently being pressed

  uint8_t delay_timer;
  uint8_t sound_timer;
} Chip8;

void chip8_init(Chip8 *self);
void chip8_cycle(Chip8 *self);
void chip8_increment_pc(Chip8 *self); // increment by instruction_size = 2 bytes
void chip8_load_rom(Chip8 *self, const char *file_path);
void chip8_debug(Chip8 *self);

#endif // CHIP8_H