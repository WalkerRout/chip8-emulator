
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONTSET_SIZE 80
#define INSTRUCTION_SIZE 2 // 2 bytes

#include "chip8.h"
#include "instruction.h"

const unsigned char chip8_fontset[FONTSET_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

static inline void decrement_timers(Chip8 *cpu);
static inline void increment_pc(Chip8 *cpu);
static void step_cycle(Chip8 *cpu);
static void step_cycle_alu(InstructionALU op, Chip8 *cpu);
static void step_cycle_misc(InstructionMISC op, Chip8 *cpu);

void chip8_init(Chip8 *self) {
  // initialize state info
  self->opcode = 0;
  self->index = 0;
  self->sp = 0;
  self->pc = 0x200;
  self->delay_timer = 0;
  self->sound_timer = 0;

  // initialize memory
  memset(self->memory, 0, MEMORY_SIZE * sizeof(uint8_t));
  memset(self->graphics, 0, GRAPHICS_SIZE * sizeof(uint8_t));
  memset(self->stack, 0, STACK_SIZE * sizeof(uint16_t));
  memset(self->registers, 0, REGISTER_COUNT * sizeof(uint8_t));
  memset(self->keys, 0, KEY_COUNT * sizeof(uint16_t));

  // initialize fonts
  for(uint32_t i = 0; i < FONTSET_SIZE; ++i)
    self->memory[i] = chip8_fontset[i];
}

inline void chip8_cycle(Chip8 *self) {
  if(self->pc+INSTRUCTION_SIZE > 0x999) {
    eprintf("Error - program counter out of memory bounds\n");
    return;
  }

  // move current memory data into opcode
  self->opcode = self->memory[self->pc] << 8 | self->memory[self->pc+1]; // ie.. 1234 5678
  step_cycle(self);
}

// file local functions

// this is basically all the logic for opcodes
static void step_cycle(Chip8 *cpu) {
  Instruction first_nibble = (uint8_t) (cpu->opcode >> 12); // ie.. 1110 1011 0011 1100 -> 0000 0000 0000 1110

  // set these directly to (cpu->opcode & 0x0F00) >> 8 and (cpu->opcode & 0x00F0) >> 4
  uint8_t x = (cpu->opcode & 0x0F00) >> 8;
  uint8_t y = (cpu->opcode & 0x00F0) >> 4;

  switch(first_nibble) {
  case INSTRUCTION_CLS_RET: // cls & ret
    if(cpu->opcode == 0x00E0) {
      // clear screen
      memset(cpu->graphics, 0, GRAPHICS_SIZE * sizeof(uint8_t));
    } else if (cpu->opcode == 0x00EE) {
      // return from subroutine
      cpu->sp -= 1;
      cpu->pc = cpu->stack[cpu->sp];
    } else {
      eprintf("Error - unimplemented opcode 0x%.4x\n", cpu->opcode);
    }

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_JP1:
    // 0x1nnn -> set nnn to be jump addr
    cpu->pc = cpu->opcode & 0x0FFF;
    decrement_timers(cpu);
    break;

  case INSTRUCTION_CALL:
    cpu->stack[cpu->sp] = cpu->pc;
    cpu->sp += 1;
    cpu->pc = cpu->opcode & 0x0FFF;
    decrement_timers(cpu);
    break;

  case INSTRUCTION_SE1:
    if(cpu->registers[x] == (cpu->opcode & 0x00FF))
      increment_pc(cpu);

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_SNE1:
    if(cpu->registers[x] != (cpu->opcode & 0x00FF))
      increment_pc(cpu);

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_SE2:
    if(cpu->registers[x] == cpu->registers[y])
      increment_pc(cpu);

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_LD1:
    cpu->registers[x] = (uint8_t) (cpu->opcode & 0x00FF);
    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_ADD:
    cpu->registers[x] += (uint8_t) (cpu->opcode & 0x00FF);
    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_ALU_SET:;
    InstructionALU alu_op = (uint8_t) (cpu->opcode & 0x000F);
    step_cycle_alu(alu_op, cpu);
    break;

  case INSTRUCTION_SNE2:
    if(cpu->registers[x] != cpu->registers[y])
      increment_pc(cpu);

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_LD2:
    cpu->index = cpu->opcode & 0x0FFF;
    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_JP2:
    cpu->pc = (cpu->opcode & 0x0FFF) + (uint16_t) cpu->registers[0];
    decrement_timers(cpu);
    break;

  case INSTRUCTION_RND:
    cpu->registers[x] = (uint8_t) ((rand() % 8) & (cpu->opcode & 0x00FF));

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_DRW:
    cpu->registers[0xF] = 0;

    uint8_t height = cpu->opcode & 0x000F;

    uint8_t reg_x = cpu->registers[x];
    uint8_t reg_y = cpu->registers[y];

    for(uint32_t i = 0; i < height; ++i) {
      uint8_t pixel = cpu->memory[cpu->index + i];

      // for each bit...
      for(uint32_t j = 0; j < 8; ++j) {
        uint8_t msb = 0x80;

        // 0b01010101
        if((pixel & (msb >> j)) != 0) {
          uint8_t ty = (reg_y + i) % 32; // 32-height
          uint8_t tx = (reg_x + j) % 64; // 64-width
          uint32_t index = ty*64 + tx;

          // flip index bits
          cpu->graphics[index] ^= 1;

          // can only do this because of previous if-statement check (only runs on bits that are != 0)
          if(cpu->graphics[index] == 0)
            cpu->registers[0xF] = 1;
        }
      }
    }

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_SKP_SKPN:;
    uint8_t skp_op = cpu->opcode & 0x00FF;

    if(skp_op == 0x9E) {
      if(cpu->keys[cpu->registers[x]] == 1)
        increment_pc(cpu);
    } else if (skp_op == 0xA1) {
      if(cpu->keys[cpu->registers[x]] != 1)
        increment_pc(cpu);
    }

    decrement_timers(cpu);
    increment_pc(cpu);
    break;

  case INSTRUCTION_MISC_SET:;
    InstructionMISC misc_op = (uint8_t) (cpu->opcode & 0x00FF);
    step_cycle_misc(misc_op, cpu);
    break;

  default: {}
  }
}

void chip8_load_rom(Chip8 *self, const char *file_path) {
  eprintf("Loading ROM from %s...\n", file_path);

  FILE *file = fopen(file_path, "rb");
  if(file == NULL) {
    eprintf("Error - failed to load file `%s`\n", file_path);
    goto fail;
  }

  if(fseek(file, 0, SEEK_END) < 0) goto fail;
  
  long file_size = ftell(file);
  if(file_size < 0) goto fail;

  if(fseek(file, 0, SEEK_SET) < 0) goto fail;

  if(self->pc + file_size > MEMORY_SIZE) goto fail;

  fread(&self->memory[self->pc], 1, file_size, file);
  if(ferror(file)) goto fail;

  eprintf("Successfully loaded ROM\n");

fail:
  if(file) fclose(file);
}

void chip8_debug(Chip8 *self) {
  eprintf("---- Memory ----\n");
  for(uint32_t i = 1; i <= MEMORY_SIZE; ++i){
    if(i == 0x200) eprintf("\n\t-- 0x200 --\n\n");
    eprintf("0x%.2x%c", self->memory[i-1], i % 32 ? ' ' : '\n');
  }

  eprintf("\n---- Graphics ----\n");
  for(uint32_t i = 1; i <= GRAPHICS_SIZE; ++i)
    eprintf("0x%.2x%c", self->graphics[i-1], i % 16 ? ' ' : '\n');

  eprintf("\n---- Registers ----\n");
  for(uint32_t i = 1; i <= REGISTER_COUNT; ++i)
    eprintf("0x%.2x%c", self->registers[i-1], i % 4 ? ' ' : '\n');

  eprintf("\n---- Stack ----\n");
  for(uint32_t i = 1; i <= STACK_SIZE; ++i)
    eprintf("0x%.2x\n", self->stack[i-1]);

  eprintf("\nIndex: %u\nSP: %u\nPC: %u\n", self->index, self->sp, self->pc);
}

static inline void decrement_timers(Chip8 *cpu) {
  if(cpu->delay_timer > 0) 
    cpu->delay_timer -= 1;

  if(cpu->sound_timer > 0) {
    // TODO; sound
    cpu->sound_timer -= 1;
  }
}

static inline void increment_pc(Chip8 *cpu) {
  cpu->pc += INSTRUCTION_SIZE;
}

static void step_cycle_alu(InstructionALU op, Chip8 *cpu) {
  uint8_t x = (cpu->opcode & 0x0F00) >> 8;
  uint8_t y = (cpu->opcode & 0x00F0) >> 4;

  switch(op) {
  case INSTRUCTION_ALU_LD:
    cpu->registers[x] = cpu->registers[y];
    break;

  case INSTRUCTION_ALU_OR:
    cpu->registers[x] |= cpu->registers[y];
    break;

  case INSTRUCTION_ALU_AND:
    cpu->registers[x] &= cpu->registers[y];
    break;

  case INSTRUCTION_ALU_XOR:
    cpu->registers[x] ^= cpu->registers[y];
    break;

  case INSTRUCTION_ALU_ADD:;
    uint16_t overflow = cpu->registers[x] + cpu->registers[y];
    cpu->registers[0xF] = overflow > 255 ? 1 : 0;
    cpu->registers[x] = (uint8_t) (overflow & 0x00FF);
    break;

  case INSTRUCTION_ALU_SUB:
    // set registerF to NOT borrow (~borrow)
    cpu->registers[0xF] = cpu->registers[x] > cpu->registers[y] ? 1 : 0;
    cpu->registers[x] -= cpu->registers[y];
    break;

  case INSTRUCTION_ALU_SHR:
    // check LSB
    cpu->registers[0xF] = cpu->registers[x] & 0b00000001;
    cpu->registers[x] >>= 1;
    break;

  case INSTRUCTION_ALU_SUBN:
    cpu->registers[0xF] = cpu->registers[y] > cpu->registers[x] ? 1 : 0;
    cpu->registers[x] = cpu->registers[y] - cpu->registers[x];
    break;

  case INSTRUCTION_ALU_SHL:
    // check MSB (0x80 = 128)
    cpu->registers[0xF] = (cpu->registers[x] & 0b10000000) != 0 ? 1 : 0;
    cpu->registers[x] <<= 1;
    break;

  default: {}
  }

  decrement_timers(cpu);
  increment_pc(cpu);
}

static void step_cycle_misc(InstructionMISC op, Chip8 *cpu) {
  uint8_t x = (cpu->opcode & 0x0F00) >> 8;

  switch(op) {
  case INSTRUCTION_MISC_LD1:
    cpu->registers[x] = cpu->delay_timer;
    break;

  case INSTRUCTION_MISC_LD2:;
    // will waste cycles...
    uint8_t key_pressed = FALSE;
    for(uint32_t i = 0; i < KEY_COUNT; ++i) {
      if(cpu->keys[i] != 0){
        cpu->registers[x] = (uint8_t) i; // record which key was pressed
        key_pressed = TRUE;
        break;
      }
    }

    // return immediately, do not run increment_pc()
    if(!key_pressed) return;
    break;

  case INSTRUCTION_MISC_LD3:
    cpu->delay_timer = cpu->registers[x];
    break;

  case INSTRUCTION_MISC_LD4:
    cpu->sound_timer = cpu->registers[x];
    break;

  case INSTRUCTION_MISC_ADD:
    cpu->index += cpu->registers[x];
    break;

  case INSTRUCTION_MISC_LD5:
    // chip8 font-set starts at cpu->memory[0]
    if(cpu->registers[x] < 16)
      cpu->index = cpu->registers[x] * 5; // skip between chars, each 5 bytes
    break;

  case INSTRUCTION_MISC_LD6:
    // store BCD representation of Vx
    cpu->memory[cpu->index+0] = cpu->registers[x] / 100; // hundreds digit
    cpu->memory[cpu->index+1] = (cpu->registers[x] / 10) % 10; // tens digit
    cpu->memory[cpu->index+2] = cpu->registers[x] % 10; // ones digit
    break;

  case INSTRUCTION_MISC_LD7:
    // dump registers up to x (inclusive) into memory
    for(uint32_t i = 0; i <= x; ++i)
      cpu->memory[cpu->index + i] = cpu->registers[i];
    break;

  case 0x65:
    // load memory into registers up to x (inclusive)
    for(uint32_t i = 0; i <= x; ++i)
      cpu->registers[i] = cpu->memory[cpu->index + i];
    break;

  default: {}
  }

  decrement_timers(cpu);
  increment_pc(cpu);
}