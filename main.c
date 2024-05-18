#include <unistd.h> /* issaty */
#include <signal.h>
#include "lc3.h"

void handle_interrupt(int s)
{
    (void)(s);
    restore_input_buffering();
    write(1, "\n", 1);
    exit(6);
}

int main(int argc, char **argv)
{
	int running = 1, i;
	uint16_t *memory, *reg;

    if (argc < 2) {
        /* show usage string */
        fprintf(stderr, "Usage: lc3 [image-file1] ...\n");
        return 1;
    }

    if(!isatty(0)) {
        fprintf(stderr, "Not a terminal, sorry\n");
        return 2;
    }

    memory = initialize_memory();
    if(!memory) {
        fprintf(stderr, "Failed to initialize_memory\n");
        return 3;
    }

    reg = initialize_registers();
    if(!reg) {
        fprintf(stderr, "Failed to initialize_registers\n");
        free(memory);
        return 4;
    }

	for (i = 1; i < argc; i++) {
    	if (!read_image(argv[i], memory)) {
        	fprintf(stderr, "Failed to load image: %s\n", argv[i]);
            free_resources(memory, reg);
        	return 5;
    	}
	}

    disable_input_buffering();
    signal(SIGINT, handle_interrupt);

	while (running) {
        /* steps:
        1. load one instruction from memory at the address of the PC register
        2. increment the PC register
        3. look at the opcode to determine which type of instruction it should perform.
        4. perform the instruction using the parameters in the instruction
        5. go back to step 1 */
        uint16_t op;
        uint16_t instr;
        instr = mem_read(reg[R_PC], memory);
        reg[R_PC]++;
        /* ADD R2 R0 R1 = big-endian = 0000 0001 0001 0100
        swap to little-endian = 0001 0100 0000 0001
        instr = 0001 0100 0000 0001 >> 12 */
        op = instr >> 12;

        switch (op) {
            case OP_ADD:
            	op_add(reg, instr);
    			break;
            case OP_AND:
                op_and(reg, instr);	
                break;
            case OP_NOT:
                op_not(reg, instr);
    			break;
            case OP_BR:
                op_br(reg, instr);
                break;
            case OP_JMP:
                op_jmp(reg, instr);
                break;
            case OP_JSR:
                op_jsr(reg, instr);
                break;
            case OP_LD:
                op_ld(reg, instr, memory);
                break;
            case OP_LDI:
                op_ldi(reg, instr, memory);
                break;
            case OP_LDR:
                op_ldr(reg, instr, memory);
                break;
            case OP_LEA:
                op_lea(reg, instr);
                break;
            case OP_ST:
                op_st(reg, instr, memory);
                break;
            case OP_STI:
                op_sti(reg, instr, memory);
                break;
            case OP_STR:
                op_str(reg, instr, memory);
                break;
            case OP_TRAP:
                reg[R_R7] = reg[R_PC];
				switch (instr & 0xFF)
				{
    				case TRAP_GETC:
        				/* read a single ASCII char */
						reg[R_R0] = (uint16_t)getchar();
						update_flags(R_R0, reg);
        				break;
    				case TRAP_OUT:
        				putc((char)reg[R_R0], stdout);
						fflush(stdout);
        				break;
    				case TRAP_PUTS:
        				trap_puts(reg, memory);
        				break;
    				case TRAP_IN:
        				trap_in(reg);					
        				break;
    				case TRAP_PUTSP:
        				trap_putsp(reg, memory);
        				break;
    				case TRAP_HALT:
        				puts("HALT");
						fflush(stdout);
						running = 0;
        				break;
				}
                break;
            case OP_RES:
            case OP_RTI:
            default:
                running = 0;
                break;
        }
    }
    free_resources(memory, reg);
    restore_input_buffering();
	return 0;
}
