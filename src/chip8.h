#ifndef CHIP8_H
#define CHIP8_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#if __STDC_VERSION__ < 202000L
    typedef enum bool {false=0, true=1} bool;
#endif

#define CHIP8_PROG_ENTRY     0x200
#define CHIP8_PROG_MAX_SIZE  0xE00
#define CHIP8_FONTSET_ADDR   0x000
#define CHIP8_FONTSET_STRIDE 5
#define CHIP8_FONTSET_SIZE   80
#define CHIP8_SCREEN_X       64
#define CHIP8_SCREEN_Y       32
#define CHIP8_SCREEN_SIZE    CHIP8_SCREEN_X * CHIP8_SCREEN_Y
#define CHIP8_KEY_COUNT      16
#define CHIP8_V_REG_COUNT    16

typedef struct chip8_t
{
    uint16_t stack  [16];
    uint8_t  screen [64][32];
    uint8_t  memory [4096];
    uint8_t  keys   [16];
    uint8_t  reg_v  [16];

    uint16_t reg_pc;
    uint16_t reg_i;
    uint8_t  reg_dt;
    uint8_t  reg_st;
    uint8_t  reg_sp;

    bool step_flag;
} chip8_t;

// Initializes the chip8 interpreter with a file descriptor for the ROM.
void chip8_init (chip8_t* chip8, FILE* fp);

// Executes a single chip8 instruction.
void chip8_exec_next (chip8_t* chip8);

// Returns the opcode at the current instruction pointer.
uint16_t chip8_get_opcode (chip8_t* chip8);

// Decrements the chip8 DT & ST timers.
void chip8_dec_timers (chip8_t* chip8);

// Executes a chip8 draw command to the screen buffer.
void chip8_draw_screen (chip8_t* chip8, uint8_t x, uint8_t y, uint8_t n);

// Prints the chip8 external state to STDOUT.
void chip8_print_debug (chip8_t* chip8);

#endif // CHIP8_H