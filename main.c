#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

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

#define MAX_MEM 65536

struct Mem
{
    Byte data[MAX_MEM]; // Max memory.
};

struct Cpu
{
    Word pc; // Program Counter.
    Byte sp; // Stack Pointer.

    Byte a; // Accumulator.
    Byte x; // Index Register X.
    Byte y; // Index Register Y.

    // Status flags.
    Byte c : 1;
    Byte z : 1;
    Byte i : 1;
    Byte d : 1;
    Byte b : 1;
    Byte v : 1;
    Byte n : 1;

    // Opcodes.
    Byte ins_lda_im; // Load accumulator immediate mode.
    Byte ins_lda_zp; // Zero page.
    Byte ins_lda_zpx; // Zero page.X.
    Byte ins_jsr;
};

void
init_mem(struct Mem *mem)
{
    u32 max_mem = MAX_MEM;

    for(u32 i = 0; i < max_mem; ++i)
    {
        mem->data[i] = 0;
    }
}

// See: https://c64-wiki.com/wiki/Reset_(Process)
void
reset(struct Cpu *cpu, struct Mem *mem)
{
    cpu->pc = 0xfffc; // fffc is the 6052 reset vector.
    cpu->sp = 0x00;

    cpu->a = 0x00;
    cpu->x = 0x00;
    cpu->y = 0x00;

    cpu->c = 0;
    cpu->z = 0;
    cpu->i = 0;
    cpu->d = 0;
    cpu->b = 0;
    cpu->v = 0;
    cpu->n = 0;

    cpu->ins_lda_im = 0xa9;
    cpu->ins_lda_zp = 0xa5;
    cpu->ins_lda_zpx = 0xb5;

    init_mem(mem);
}

Byte
fetch_byte(struct Cpu *cpu, struct Mem *mem, i32 *cycles)
{
    Byte data = mem->data[cpu->pc];
    cpu->pc++;
    (*cycles)--;
    return data;
}

Word
fetch_word(struct Cpu *cpu, struct Mem *mem, i32 *cycles)
{
    Word data = mem->data[cpu->pc];
    cpu->pc++;

    data |= (mem->data[cpu->pc] << 8);
    cpu->pc++;
    (*cycles) += 2;
    return data;
}

void
write_word(Word addr, struct Mem *mem, i32 *cycles)
{
    mem->data[addr] = addr & 0xFF;
    mem->data[addr + 1] = (addr >> 8);
    (*cycles) -= 2;
}

// Like fetch_byte but does not increment the program counter PC.
Byte
read_byte(struct Mem *mem, i32 *cycles, Byte addr)
{
    Byte data = mem->data[addr];
    (*cycles)--;
    return data;
}

i32
execute(struct Cpu *cpu, struct Mem *mem, i32 cycles)
{
    i32 requested_cycles = cycles;

    while(cycles > 0)
    {
        Byte ins = fetch_byte(cpu, mem, &cycles);
        printf("%d\n", ins);

        if(ins == cpu->ins_lda_im)
        {
            printf("blip\n");
            Byte value = fetch_byte(cpu, mem, &cycles);
            cpu->a = value;
            cpu->z = (cpu->a == 0); // Set Z to 1 if A == 0.
            cpu->n = (cpu->a & 0b10000000) > 0;
        }
        else if(ins == cpu->ins_lda_zp)
        {
            printf("blop\n");
            Byte zero_page_addr = fetch_byte(cpu, mem, &cycles);
            cpu->a = read_byte(mem, &cycles, zero_page_addr);
            cpu->z = (cpu->a == 0);
            cpu->n = (cpu->a & 0b10000000) > 0;
        }
        else if(ins == cpu->ins_lda_zpx)
        {
            printf("blap\n");
            Byte zero_page_addr = fetch_byte(cpu, mem, &cycles);
            zero_page_addr += cpu->x;
            cycles--;
            cpu->a = read_byte(mem, &cycles, zero_page_addr);
            cpu->z = (cpu->a == 0);
            cpu->n = (cpu->a & 0b10000000) > 0;
        }
        else if(ins == cpu->ins_jsr)
        {
            printf("blup\n");
            Word sub_addr = fetch_word(cpu, mem, &cycles);
            write_word(cpu->pc - 1, mem, &cycles);
            cpu->pc = sub_addr;
            cycles--;
        }
        else
        {
            printf("Unrecognized instruction: %d\n" , ins);
        }
    }
    // Return the number of cycles used.
    return requested_cycles - cycles;
}

// Zero cycles test.
void
test_zero_cycles(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);
    i32 nb_cycles = execute(cpu, mem, 0);
    assert(nb_cycles == 0);
}

void
test_full_cycle(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    mem->data[0xfffc] = cpu->ins_lda_im;
    mem->data[0xfffd] = 0x42;

    i32 nb_cycles = execute(cpu, mem, 1);

    assert(nb_cycles == 2);
}

void
test_invalid_instruction(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    mem->data[0xfffc] = 0x1;
    mem->data[0xfffd] = 0x1;

    execute(cpu, mem, 2);
    i32 nb_cycles = execute(cpu, mem, 2);

    assert(nb_cycles == 2);
}

// LDA Immediate mode: load value into the A register.
void
test_lda_im(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    mem->data[0xfffc] = cpu->ins_lda_im;
    mem->data[0xfffd] = 0x42;

    i32 nb_cycles = execute(cpu, mem, 2);

    assert(nb_cycles == 2);
    assert(cpu->a == 0x42);

    assert(cpu->c == 0);
    assert(cpu->z == 0);
    assert(cpu->i == 0);
    assert(cpu->d == 0);
    assert(cpu->b == 0);
    assert(cpu->v == 0);
    assert(cpu->n == 0);
}

// LDA Zero Page: load value at a specific address into the A register.
void
test_lda_zp(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    mem->data[0xfffc] = cpu->ins_lda_zp;
    mem->data[0xfffd] = 0x42;
    mem->data[0x0042] = 0x84;

    i32 nb_cycles = execute(cpu, mem, 3);

    assert(nb_cycles == 3);
    assert(cpu->a == 0x84);

    assert(cpu->c == 0);
    assert(cpu->z == 0);
    assert(cpu->i == 0);
    assert(cpu->d == 0);
    assert(cpu->b == 0);
    assert(cpu->v == 0);
    assert(cpu->n == 1);
}

// LDA Zero Page X: load value at a specific address into the A register.
void
test_lda_zp_x(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    cpu->x = 5;

    mem->data[0xfffc] = cpu->ins_lda_zpx;
    mem->data[0xfffd] = 0x42;;
    mem->data[0x0047] = 0x37;

    i32 nb_cycles = execute(cpu, mem, 4);

    assert(nb_cycles == 4);
    assert(cpu->a == 0x37);

    assert(cpu->c == 0);
    assert(cpu->z == 0);
    assert(cpu->i == 0);
    assert(cpu->d == 0);
    assert(cpu->b == 0);
    assert(cpu->v == 0);
    assert(cpu->n == 0);
}

void
test_lda_zp_x_2(struct Cpu *cpu, struct Mem *mem)
{
    reset(cpu, mem);

    cpu->x = 0xff;

    mem->data[0xfffc] = cpu->ins_lda_zpx;
    mem->data[0xfffd] = 0x80;;
    mem->data[0x007f] = 0x37;

    i32 nb_cycles = execute(cpu, mem, 4);

    assert(nb_cycles == 4);
    assert(cpu->a == 0x37);

    assert(cpu->c == 0);
    assert(cpu->z == 0);
    assert(cpu->i == 0);
    assert(cpu->d == 0);
    assert(cpu->b == 0);
    assert(cpu->v == 0);
    assert(cpu->n == 0);
}


/* void */
/* test_4(struct Cpu *cpu, struct Mem *mem) */
/* { */
/*     reset(cpu, mem); */

/*     // 9 cycles. */
/*     mem->data[0xfffc] = cpu->ins_jsr; */
/*     mem->data[0xfffd] = 0x42; */
/*     mem->data[0xfffe] = 0x42; */
/*     mem->data[0x4242] = cpu->ins_lda_im; */
/*     mem->data[0x4243] = 0x84; */

/*     execute(cpu, mem, 10); */

/*     assert(cpu->a == 0x86); */
/* } */


int
main(void)
{
    struct Mem mem;
    struct Cpu cpu;

    /* test_zero_cycles(&cpu, &mem); */

    /* test_full_cycle(&cpu, &mem); */

    test_invalid_instruction(&cpu, &mem);

    /* test_lda_im(&cpu, &mem); */

    /* test_lda_zp(&cpu, &mem); */

    /* test_lda_zp_x(&cpu, &mem); */

    /* test_lda_zp_x_2(&cpu, &mem); */

    return 0;
}
