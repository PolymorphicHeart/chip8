#include "chip8.h"

static uint8_t FONTSET_BUFFER[80] = 
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // '0'
    0x20, 0x60, 0x20, 0x20, 0x70, // '1'
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // '2'
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // '3'
    0x90, 0x90, 0xF0, 0x10, 0x10, // '4'
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // '5'
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // '6'
    0xF0, 0x10, 0x20, 0x40, 0x40, // '7'
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // '8'
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // '9'
    0xF0, 0x90, 0xF0, 0x90, 0x90, // 'A'
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // 'B'
    0xF0, 0x80, 0x80, 0x80, 0xF0, // 'C'
    0xE0, 0x90, 0x90, 0x90, 0xE0, // 'D'
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // 'E'
    0xF0, 0x80, 0xF0, 0x80, 0x80, // 'F'
};

void chip8_init (chip8_t* chip8, FILE* fp)
{
    srand(time(NULL));

    *chip8 = (chip8_t)
    {
        .stack  = {0},
        .screen = {0},
        .memory = {0},
        .keys   = {0},
        .reg_v  = {0},

        .reg_pc = CHIP8_PROG_ENTRY,
        .reg_i  = 0,
        .reg_dt = 0,
        .reg_st = 0,
        .reg_sp = 0
    };

    memmove(chip8->memory, FONTSET_BUFFER, CHIP8_FONTSET_SIZE);
    fread(chip8->memory + CHIP8_PROG_ENTRY, sizeof(uint8_t), CHIP8_PROG_MAX_SIZE, fp);
}

void chip8_exec_next (chip8_t* chip8)
{
    uint16_t opcode = chip8->memory[chip8->reg_pc] << 8 | chip8->memory[chip8->reg_pc + 1];
    uint16_t nnn    = opcode & 0x0FFF;
    uint8_t  kk     = opcode & 0x00FF;
    uint8_t  x      = (opcode >> 8) & 0x000F;
    uint8_t  y      = (opcode >> 4) & 0x000F;
    uint8_t  n      = opcode & 0x000F;

    switch (opcode & 0xF000)
    {
        case 0x0000:
            switch (kk)
            {
                case 0x00E0: // 00E0 - CLS
                    memset(chip8->screen, 0, sizeof(uint8_t) * CHIP8_SCREEN_SIZE);
                    break;

                case 0x00EE: // 00EE - RET
                    chip8->reg_pc = chip8->stack[--chip8->reg_sp];
                    return;
            }
            break;
        
        case 0x1000: // 1nnn - JP addr
            chip8->reg_pc = nnn;
            return;
        
        case 0x2000: // 2nnn - CALL addr
            chip8->stack[chip8->reg_sp++] = chip8->reg_pc + 2;
            chip8->reg_pc = nnn;
            return;
        
        case 0x3000: // 3xkk - SE Vx, byte
            chip8->reg_pc += (chip8->reg_v[x] == kk) ? 4 : 2;
            return;
        
        case 0x4000: // 4xkk - SNE Vx, byte
            chip8->reg_pc += (chip8->reg_v[x] == kk) ? 2 : 4;
            return;

        case 0x5000: // 5xy0 - SE Vx, Vy
            chip8->reg_pc += (chip8->reg_v[x] == chip8->reg_v[y]) ? 4 : 2;
            return;

        case 0x6000: // 6xkk - LD Vx, byte
            chip8->reg_v[x] = kk;
            break;
        
        case 0x7000: // 7xkk - ADD Vx, byte
            chip8->reg_v[x] += kk;
            break;
        
        case 0x8000:
            switch (n)
            {
                case 0x0: // 8xy0 - LD Vx, Vy
                    chip8->reg_v[x] = chip8->reg_v[y];
                    break;
                
                case 0x1: // 8xy1 - OR Vx, Vy
                    chip8->reg_v[x] |= chip8->reg_v[y];
                    break;

                case 0x2: // 8xy2 - AND Vx, Vy
                    chip8->reg_v[x] &= chip8->reg_v[y];
                    break;

                case 0x3: // 8xy3 - XOR Vx, Vy
                    chip8->reg_v[x] ^= chip8->reg_v[y];
                    break;

                case 0x4: // 8xy4 - ADD Vx, Vy
                    chip8->reg_v[0xF] = (((int16_t) chip8->reg_v[x]) + ((int16_t) chip8->reg_v[y]) > 255) ? 1 : 0;
                    chip8->reg_v[x] += chip8->reg_v[y];
                    break;
                
                case 0x5: // 8xy5 - SUB Vx, Vy
                    //TODO: investigate this:
                    chip8->reg_v[0xF] = (chip8->reg_v[x] > chip8->reg_v[y]) ? 1 : 0;
                    chip8->reg_v[x] -= chip8->reg_v[y];
                    break;
                
                case 0x6: // 8xy6 - SHR Vx {, Vy}
                    chip8->reg_v[0xF] = chip8->reg_v[x] & 0x1;
                    chip8->reg_v[x] = (chip8->reg_v[x] >> 1);
                    break;
                
                case 0x7: // 8xy7 - SUBN Vx, Vy
                    chip8->reg_v[0xF] = (chip8->reg_v[y] > chip8->reg_v[x]) ? 1 : 0;
                    chip8->reg_v[x] = chip8->reg_v[y] - chip8->reg_v[x];
                    break;
                
                case 0xE: // 8xyE - SHL Vx {, Vy}
                    chip8->reg_v[0xF] = (chip8->reg_v[x] >> 7) & 0x1;
                    chip8->reg_v[x] = (chip8->reg_v[x] << 1);
                    break;
            }
            break;
        
        case 0x9000: // 9xy0 - SNE Vx, Vy
            chip8->reg_pc += (chip8->reg_v[x] == chip8->reg_v[y]) ? 2 : 4;
            return;
        
        case 0xA000: // Annn - LD I, addr
            chip8->reg_i = nnn;
            break;
        
        case 0xB000: // Bnnn - JP V0, addr
            chip8->reg_pc = nnn + chip8->reg_v[0];
            return;
        
        case 0xC000: // Cxkk - RND Vx, byte
            chip8->reg_v[x] = (rand() % 256) % kk;
            break;
        
        case 0xD000: // Dxyn - DRW Vx, Vy, nibble
            chip8_draw_screen(chip8, x, y, n);
            break;
        
        case 0xE000:
            switch (kk)
            {
                case 0x9E: // Ex9E - SKP Vx
                    chip8->reg_pc += (chip8->keys[chip8->reg_v[x]]) ? 4 : 2;
                    return;

                case 0xA1: // ExA1 - SKNP Vx
                    chip8->reg_pc += (chip8->keys[chip8->reg_v[x]]) ? 2 : 4;
                    return;
            }
            break;
        
        case 0xF000:
            switch (kk)
            {
                case 0x07: // Fx07 - LD Vx, DT
                    chip8->reg_v[x] = chip8->reg_dt;
                    break;
                
                case 0x0A: // Fx0A - LD Vx, K
                    for (int8_t k = 0; k < CHIP8_KEY_COUNT; k++)
                    {
                        if (chip8->keys[k])
                        {
                            chip8->reg_v[x] = k;
                            chip8->reg_pc += 2;
                            return;
                        }
                    }
                    return;
                
                case 0x15: // Fx15 - LD DT, Vx
                    chip8->reg_dt = chip8->reg_v[x];
                    break;
                
                case 0x18: // Fx18 - LD ST, Vx
                    chip8->reg_st = chip8->reg_v[x];
                    break;
                
                case 0x1E: // Fx1E - ADD I, Vx
                    chip8->reg_v[0xF] = (chip8->reg_i + chip8->reg_v[x] > 0xFFF) ? 1 : 0;
                    chip8->reg_i += chip8->reg_v[x];
                    break;

                case 0x29: // Fx29 - LD F, Vx
                    chip8->reg_i = CHIP8_FONTSET_STRIDE * chip8->reg_v[x];
                    break;
                
                case 0x33: // Fx33 - LD B, Vx
                    chip8->memory[chip8->reg_i]     = (chip8->reg_v[x] % 1000) / 100;
                    chip8->memory[chip8->reg_i + 1] = (chip8->reg_v[x] % 100) / 10;
                    chip8->memory[chip8->reg_i + 2] = (chip8->reg_v[x] % 10);
                    break;
                
                case 0x55: // Fx55 - LD [I], Vx
                    //TODO: check for mem overflow!
                    memmove(chip8->memory + chip8->reg_i, chip8->reg_v, sizeof(uint8_t) * x + 1);
                    chip8->reg_i += x + 1;
                    break;

                case 0x65: // Fx65 - LD Vx, [I]
                    //TODO: check for mem overflow!
                    memmove(chip8->reg_v, chip8->memory + chip8->reg_i, sizeof(uint8_t) * x + 1);
                    chip8->reg_i += x + 1;
                    break;
            }
            break;
    }

    chip8->reg_pc += 2;
}

uint16_t chip8_get_opcode (chip8_t* chip8)
{
    return chip8->memory[chip8->reg_pc] << 8 | chip8->memory[chip8->reg_pc + 1];
}

void chip8_dec_timers (chip8_t* chip8)
{
    chip8->reg_dt -= (chip8->reg_dt > 0) ? 1 : 0;
    chip8->reg_st -= (chip8->reg_st > 0) ? 1 : 0;
}

void chip8_draw_screen (chip8_t* chip8, uint8_t x, uint8_t y, uint8_t n)
{
    chip8->reg_v[0xF] = 0;
    uint8_t px = 0;

    for (uint16_t yl = 0; yl < n; yl++) 
    {
        px = chip8->memory[chip8->reg_i + yl];
        for (uint16_t xl = 0; xl < 8; xl++) 
        {
            if ((px & (0x80 >> xl)) != 0) 
            {
                uint8_t* scr_px = &chip8->screen[(chip8->reg_v[x] + xl) % CHIP8_SCREEN_X][(chip8->reg_v[y] + yl) % CHIP8_SCREEN_Y];
                if (*scr_px == 1) chip8->reg_v[0xF] = 1;
                *scr_px ^= 0x1;
            }
        }
    }
}

#define TERM_RESET     "\033[0m"
#define TERM_BG_BLACK  "\033[48;5;0m"
#define TERM_FG_PURPLE "\033[38;5;177m"
#define TERM_FG_ORANGE "\033[38;5;214m"
#define TERM_FG_WHITE  "\033[38;5;15m"

void chip8_print_debug (chip8_t* chip8)
{
    printf("%s\n", TERM_BG_BLACK);

#if defined(_WIN32)
    system("cls");
#else
    system("clear");
#endif

    uint8_t sp = chip8->reg_sp;

    printf
    (
        "  %sCHIP8 BREAKPOINT MODE%s\n\n"
        "    == REGISTERS == \n"
        "    %sOP%s %04x   %sV0%s %02x %sV6%s %02x %sVC%s %02x\n"
        "    %sPC%s 0x%03x  %sV1%s %02x %sV7%s %02x %sVD%s %02x\n"
        "    %sSP%s %02x     %sV2%s %02x %sV8%s %02x %sVE%s %02x\n"
        "    %sST%s %02x     %sV3%s %02x %sV9%s %02x %sVF%s %02x\n"
        "    %sDT%s %02x     %sV4%s %02x %sVA%s %02x\n"
        "    %sI %s %04x   %sV5%s %02x %sVB%s %02x\n\n"
        "    |== STACK ==|\n"
        "    | %sF%s > 0x%03x%s |\n"
        "    | %sE%s > 0x%03x%s |\n"
        "    | %sD%s > 0x%03x%s |\n"
        "    | %sC%s > 0x%03x%s |\n"
        "    | %sB%s > 0x%03x%s |\n"
        "    | %sA%s > 0x%03x%s |\n"
        "    | %s9%s > 0x%03x%s |\n"
        "    | %s8%s > 0x%03x%s |\n"
        "    | %s7%s > 0x%03x%s |\n"
        "    | %s6%s > 0x%03x%s |\n"
        "    | %s5%s > 0x%03x%s |\n"
        "    | %s4%s > 0x%03x%s |\n"
        "    | %s3%s > 0x%03x%s |\n"
        "    | %s2%s > 0x%03x%s |\n"
        "    | %s1%s > 0x%03x%s |\n"
        "    | %s0%s > 0x%03x%s |\n"
        "    |===========|\n",

        TERM_FG_PURPLE, TERM_FG_WHITE,
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8_get_opcode(chip8), TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x0], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0x6], TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0xC],
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_pc,           TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x1], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0x7], TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0xD],
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_sp,           TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x2], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0x8], TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0xE],
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_st,           TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x3], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0x9], TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0xF],
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_dt,           TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x4], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0xA],
        TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_i,            TERM_FG_PURPLE, TERM_FG_WHITE, chip8->reg_v[0x5], TERM_FG_PURPLE, TERM_FG_WHITE,  chip8->reg_v[0xB],

        TERM_FG_PURPLE, (sp != 0xF) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xF], TERM_FG_WHITE,
        TERM_FG_PURPLE, (sp != 0xE) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xE], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0xD) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xD], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0xC) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xC], TERM_FG_WHITE,
        TERM_FG_PURPLE, (sp != 0xB) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xB], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0xA) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0xA], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x9) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x9], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x8) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x8], TERM_FG_WHITE,
        TERM_FG_PURPLE, (sp != 0x7) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x7], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x6) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x6], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x5) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x5], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x4) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x4], TERM_FG_WHITE,
        TERM_FG_PURPLE, (sp != 0x3) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x3], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x2) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x2], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x1) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x1], TERM_FG_WHITE, 
        TERM_FG_PURPLE, (sp != 0x0) ? TERM_FG_WHITE: TERM_FG_ORANGE, chip8->stack[0x0], TERM_FG_WHITE
    );
}