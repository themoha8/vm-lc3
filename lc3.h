#include <stdio.h> /* FILE, printf ... */
#include <stdint.h> /* uint16_t */
#include <termios.h> /* tcgetattr */
#include <stdlib.h> /* malloc */
#include <unistd.h> /* read */

/* registers */
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, /* program counter */
    R_COND /* condition flags register */
};

/* condition flags */
enum
{
    FL_POS = 1 << 0, /* P */
    FL_ZRO = 1 << 1, /* Z */
    FL_NEG = 1 << 2 /* N */
};

/* opcodes */
enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap (system call) */
};

/* trap routines */
enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

enum
{
    MR_KBSR = 0xFE00, /* keyboard status */
    MR_KBDR = 0xFE02  /* keyboard data */
};

enum {
    NUMBER_OF_REGISTERS = 16,
    PC_START = 0x3000,
    MEMORY_MAX = 65536 /* 1 << 16 */
};

/* instruction set */
void op_add(uint16_t *reg, uint16_t instr);
void op_not(uint16_t *reg, uint16_t instr);
void op_and(uint16_t *reg, uint16_t instr);
void op_br(uint16_t *reg, uint16_t instr);
void op_jmp(uint16_t *reg, uint16_t instr);
void op_jsr(uint16_t *reg, uint16_t instr);
void op_ld(uint16_t *reg, uint16_t instr, uint16_t *memory);
void op_ldi(uint16_t *reg, uint16_t instr, uint16_t *memory);
void op_ldr(uint16_t *reg, uint16_t instr, uint16_t *memory);
void op_lea(uint16_t *reg, uint16_t instr);
void op_st(uint16_t *reg, uint16_t instr, uint16_t *memory);
void op_sti(uint16_t *reg, uint16_t instr, uint16_t *memory);
void op_str(uint16_t *reg, uint16_t instr, uint16_t *memory);

/* trap routines */
void trap_puts(uint16_t *reg, uint16_t *memory);
void trap_in(uint16_t *reg);
void trap_putsp(uint16_t *reg, uint16_t *memory);

void disable_input_buffering();
void restore_input_buffering();
void free_resources(uint16_t *memory, uint16_t *registers);
void update_flags(uint16_t r, uint16_t *reg);
void read_image_file(FILE *file, uint16_t *memory);
void mem_write(uint16_t address, uint16_t val, uint16_t *memory);
uint16_t sign_extend(uint16_t x, int bit_count);
uint16_t *initialize_memory();
uint16_t *initialize_registers();
uint16_t swap16(uint16_t x);
uint16_t mem_read(uint16_t address, uint16_t *memory);
int read_image(const char *image_path, uint16_t *memory);