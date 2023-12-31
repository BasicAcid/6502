#include <stdlib.h>
#include <stdlib.h>

typedef unsigned char Byte;
typedef unsigned short Word;

struct CPU
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
};

void reset(struct CPU *cpu)
{
    cpu->PC = 0XFFFC;
}

int main(void)
{

    struct CPU cpu;

    reset(&cpu);

    return 0;
}
