#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Based on this tutorial:
// https://www.youtube.com/watch?v=qJgsuQoy9bc&list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37

typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

typedef u8 Byte;
typedef u16 Word;

struct Mem
{
    Byte data[1024 * 64]; // Max memory.
};

struct Cpu
{
    Word PC; // Program Counter.
    Byte SP; // Stack Pointer.

    Byte A; // Accumulator.
    Byte X; // Index Register X.
    Byte Y; // Index Register Y.

    // Status flags.
    Byte C : 1;
    Byte Z : 1;
    Byte I : 1;
    Byte D : 1;
    Byte B : 1;
    Byte V : 1;
    Byte N : 1;

    // Opcodes.
    Byte INS_LDA_IM; // Load accumulator immediate mode.
    Byte INS_LDA_ZP; // Zero page.
};

void
init_mem(struct Mem *mem)
{
    u32 max_mem = 1024 * 64;

    for(u32 i = 0; i < max_mem; i++)
    {
        mem->data[i] = 0;
    }
}

// See: https://c64-wiki.com/wiki/Reset_(Process)
void
reset(struct Cpu *cpu, struct Mem *mem)
{
    cpu->PC = 0XFFFC; // FFFC is the 6052 reset vector.
    cpu->SP = 0X00;

    cpu->A = 0X00;
    cpu->X = 0X00;
    cpu->Y = 0X00;

    cpu->C = 0;
    cpu->Z = 0;
    cpu->I = 0;
    cpu->D = 0;
    cpu->B = 0;
    cpu->V = 0;
    cpu->N = 0;

    cpu->INS_LDA_IM = 0xA9;
    cpu->INS_LDA_ZP = 0xA5;

    init_mem(mem);
}

Byte
fetch_byte(struct Cpu *cpu, struct Mem *mem, u32 *cycles)
{
    Byte data = mem->data[cpu->PC];
    cpu->PC++;
    (*cycles)--;
    return data;
}

// Like fetch_byte but does not increment the program counter PC.
Byte
read_byte(struct Mem *mem, u32 *cycles, Byte addr)
{
    Byte data = mem->data[addr];
    (*cycles)--;
    return data;
}

void
execute(struct Cpu *cpu, struct Mem *mem, u32 cycles)
{
    while(cycles > 0)
    {
        Byte ins = fetch_byte(cpu, mem, &cycles);

        if(ins == cpu->INS_LDA_IM)
        {
            Byte value = fetch_byte(cpu, mem, &cycles);
            cpu->A = value;
            cpu->Z = (cpu->A == 0); // Set Z to 1 if A == 0.
            cpu->N = (cpu->A & 0b10000000) > 0;
        }
        if(ins == cpu->INS_LDA_ZP)
        {
            Byte zero_page_addr = fetch_byte(cpu, mem, &cycles);
            cpu->A = read_byte(mem, &cycles, zero_page_addr);
            cpu->Z = (cpu->A == 0);
            cpu->N = (cpu->A & 0b10000000) > 0;
        }
        else
        {
            printf("Unrecognized instruction: %d\n" , ins);
        }
    }
}

int
main(void)
{
    struct Mem mem;
    struct Cpu cpu;

    reset(&cpu, &mem);

    /* mem.data[0xFFFC] = cpu.INS_LDA_IM; */
    /* mem.data[0xFFFD] = 0x42; */

    mem.data[0xFFFC] = cpu.INS_LDA_ZP;
    mem.data[0xFFFD] = 0x42;
    mem.data[0x0042] = 0x84;

    execute(&cpu, &mem, 3);

    return 0;
}
