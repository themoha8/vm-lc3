#include "lc3.h"

static struct termios ts;

uint16_t *initialize_memory()
{
    uint16_t *memory = (uint16_t*) malloc(sizeof(*memory) * MEMORY_MAX);
    return memory;
}

uint16_t *initialize_registers()
{
    uint16_t *reg = (uint16_t*) calloc(NUMBER_OF_REGISTERS, sizeof(uint16_t));

    if(reg) {
        /* since exactly one condition flag should be set at any given time, set the Z flag */
        reg[R_COND] = FL_ZRO;

        /* set the PC to starting position (0x3000 is the default) */
        reg[R_PC] = PC_START;
    }
    return reg;
}

void disable_input_buffering()
{
    struct termios ts2;
    tcgetattr(0, &ts);
    ts2 = ts;
    ts2.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(0, TCSANOW, &ts2);
}

void restore_input_buffering()
{
    tcsetattr(0, TCSANOW, &ts);
}

void free_resources(uint16_t *memory, uint16_t *registers)
{
    free(memory);
    free(registers);
}

/* from big-endian to little-endian
example: 0100 0111 1111 0101
(x << 8) 1111 0101 0000 0000
(x >> 8) 0000 0000 0100 0111
1111 0101 0000 0000 OR 0000 0000 0100 0111
1111 0101 0100 0111
*/
uint16_t swap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

void read_image_file(FILE *file, uint16_t *memory)
{
    uint16_t max_read;
    uint16_t *tmp;
    size_t read;

    /* the origin tells us where in memory to place the image */
    /*read first two bytes */
    uint16_t origin;
    fread(&origin, sizeof(origin), 1, file);
    origin = swap16(origin);

    /* we know the maximum file size so we only need one fread */
    max_read = MEMORY_MAX - origin;
    tmp = memory + origin;
    read = fread(tmp, sizeof(uint16_t), max_read, file);

    /* swap to little endian */
    while (read > 0) {
        *tmp = swap16(*tmp);
        tmp++;
        read--;
    }
}

int read_image(const char *image_path, uint16_t *memory)
{
    FILE *file = fopen(image_path, "rb");
    if (!file) { 
        return 0; 
    };
    read_image_file(file, memory);
    fclose(file);
    return 1;
}

void mem_write(uint16_t address, uint16_t val, uint16_t *memory)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address, uint16_t *memory)
{
    int check;
    char ch;
    if (address == MR_KBSR) {
        check = read(0, &ch, 1);
        if (check) {
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = ch;
        } else
            memory[MR_KBSR] = 0;
    }
    return memory[address];
}

void update_flags(uint16_t r, uint16_t *reg)
{
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    } else if (reg[r] >> 15) {
        reg[R_COND] = FL_NEG;
    } else {
        reg[R_COND] = FL_POS;
    }
}

void trap_putsp(uint16_t *reg, uint16_t *memory)
{
    /* one char per byte (two bytes per word) */
    /* here we need to swap back to */
    /* big endian format */
    uint16_t *c = memory + reg[R_R0];
    while (*c) {
        char char2;
        char char1 = (*c) & 0xFF;
        putc(char1, stdout);
        char2 = (*c) >> 8;
        if (char2)
            putc(char2, stdout);
        c++;
    }
    fflush(stdout);
}

void trap_in(uint16_t *reg)
{
    char c;
    printf("Enter a character: ");
    c = getchar();
    putc(c, stdout);
    fflush(stdout);
    reg[R_R0] = (uint16_t)c;
    update_flags(R_R0, reg); 
}

void trap_puts(uint16_t *reg, uint16_t *memory)
{
    /* one char per word */
    uint16_t *c = memory + reg[R_R0];
    while (*c) {
        putc((char)*c, stdout);
        c++;
    }
    fflush(stdout);
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) { /* checking the sign */
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void op_str(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F, 6);
    mem_write(reg[r1] + offset, reg[r0], memory);
}

void op_ldr(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    /* destination register */
    uint16_t r0 = (instr >> 9) & 0x7;
    /* baseR */
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t offset = sign_extend(instr & 0x3F, 6);
    reg[r0] = mem_read(reg[r1] + offset, memory);
    update_flags(r0, reg);
}

void op_ldi(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    /* destination register (DR) */
    uint16_t r0 = (instr >> 9) & 0x7;
    /* PCoffset 9 */
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    /* add pc_offset to the current PC, look at that memory location to get the final address */
    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset, memory), memory);
    update_flags(r0, reg);
}

void op_sti(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    mem_write(mem_read(reg[R_PC] + pc_offset, memory), reg[r0], memory);
}

void op_st(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    mem_write(reg[R_PC] + pc_offset, reg[r0], memory);
}

void op_lea(uint16_t *reg, uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    reg[r0] = reg[R_PC] + pc_offset;
    update_flags(r0, reg);
}

void op_ld(uint16_t *reg, uint16_t instr, uint16_t *memory)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    reg[r0] = mem_read(reg[R_PC] + pc_offset, memory);
    update_flags(r0, reg);
}

void op_jsr(uint16_t *reg, uint16_t instr)
{
    uint16_t long_flag = (instr >> 11) & 1;
    reg[R_R7] = reg[R_PC];
    if (long_flag) {
        uint16_t long_pc_offset = sign_extend(instr & 0x7FF, 11);
        reg[R_PC] += long_pc_offset;  /* JSR */
    } else {
        uint16_t r1 = (instr >> 6) & 0x7;
        reg[R_PC] = reg[r1]; /* JSRR */
    }
}

void op_jmp(uint16_t *reg, uint16_t instr)
{
    /* Also handles RET */
    uint16_t r1 = (instr >> 6) & 0x7;
    reg[R_PC] = reg[r1];
}

void op_br(uint16_t *reg, uint16_t instr)
{
    int16_t pc_offset = sign_extend(instr & 0x1FF, 9);
    uint16_t cond_flag = (instr >> 9) & 0x7;
    if (cond_flag & reg[R_COND])
        reg[R_PC] += pc_offset;
}

void op_not(uint16_t *reg, uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;

    reg[r0] = ~reg[r1];
    update_flags(r0, reg);
}

void op_and(uint16_t *reg, uint16_t instr)
{
    uint16_t r0 = (instr >> 9) & 0x7;
    uint16_t r1 = (instr >> 6) & 0x7;
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] & imm5;
    } else {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] & reg[r2];
    }
    update_flags(r0, reg);               
}

void op_add(uint16_t *reg, uint16_t instr)
{
    /* destination register (DR) */
    uint16_t r0 = (instr >> 9) & 0x7;
    /* first operand (SR1) */
    uint16_t r1 = (instr >> 6) & 0x7;
    /* whether we are in immediate mode */
    uint16_t imm_flag = (instr >> 5) & 0x1;

    if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
        reg[r0] = reg[r1] + imm5;
    } else {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] + reg[r2];
    }
    update_flags(r0, reg);
}
