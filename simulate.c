#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------Provided in Project1.txt------DO NOT CHANGE------------

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5
#define HALT 6
#define NOOP 7

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
	    printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Sun integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

//-------------------------------------------------------------

void func_add(stateType*, int);
void func_nand(stateType*, int);
int func_lw(stateType*, int);
int func_sw(stateType*, int);
int func_beq(stateType*, int);
int func_jalr(stateType*, int);

int main(int argc, char* argv[]) {

    stateType *statePtr;
    FILE *inFilePtr;

    if (argc != 2) {
	    printf("usage: %s <machine-code-file>\n", argv[0]);
	    return 1;
    }

    inFilePtr = fopen(argv[1], "r");
    if (inFilePtr == NULL) {
        printf("%s: can't open file %s\n", argv[0], argv[1]);
	    perror("fopen");
	    return 1;
    }

    statePtr = calloc(1, sizeof(stateType));

    //Read machine code file into memory and print out values
    int readInt;
    fscanf(inFilePtr, "%d", &readInt);
    for (int i = 0; !feof(inFilePtr) && i < NUMMEMORY; ++i) {
        statePtr->mem[i] = readInt;
        statePtr->numMemory += 1;
        printf("memory[%d]=%d\n", i, statePtr->mem[i]);
        fscanf(inFilePtr, "%d", &readInt);
    }
    printf("\n");

    //Execute instructions loop
    int instruction, opcode, instructionCount = 0, haltCond = 0;
    while(!haltCond) {
        printState(statePtr);
        instruction = statePtr->mem[statePtr->pc];
        opcode = (instruction >> 22) & 0x7;
        switch(opcode) {
            case ADD:
                func_add(statePtr, instruction);
                break;
            case NAND:
                func_nand(statePtr, instruction);
                break;
            case LW:
                haltCond = func_lw(statePtr, instruction);
                break;
            case SW:
                haltCond = func_sw(statePtr, instruction);
                break;
            case BEQ:
                haltCond = func_beq(statePtr, instruction);
                break;
            case JALR:
                haltCond = func_jalr(statePtr, instruction);
                break;
            case HALT:
                haltCond = 1;
                statePtr->pc += 1;
                break;
            case NOOP:
                statePtr->pc += 1;
                break;
        }
        ++instructionCount;
    }

    printf("machine halted\n");
    printf("total of %d instructions executed\n", instructionCount);
    printf("final state of machine:\n");
    printState(statePtr);

    free(statePtr);

    return 0;
}

void func_add(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int destReg = instruction & 0x7;

    statePtr->reg[destReg] = statePtr->reg[regA] + statePtr->reg[regB];

    statePtr->pc += 1;
}

void func_nand(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int destReg = instruction & 0x7;

    statePtr->reg[destReg] = ~(statePtr->reg[regA] & statePtr->reg[regB]);

    statePtr->pc += 1;
}

int func_lw(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int offsetField = convertNum(instruction & 0xFFFF);
    int readAddress = statePtr->reg[regA]+offsetField;

    if(readAddress < 0 || readAddress >= NUMMEMORY) {
        printf("lw: read address out of range\n");
        return 1;
    }
    
    statePtr->reg[regB] = statePtr->mem[readAddress];

    statePtr->pc += 1;
    return 0;
}

int func_sw(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int offsetField = convertNum(instruction & 0xFFFF);
    int writeAddress = statePtr->reg[regA]+offsetField;

    if(writeAddress < 0 || writeAddress >= NUMMEMORY) {
        printf("sw: write address out of range\n");
        return 1;
    }
    else {
        statePtr->mem[writeAddress] = statePtr->reg[regB];
    }
    
    statePtr->pc += 1;
    return 0;
}

int func_beq(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int offsetField = convertNum(instruction & 0xFFFF);
    int destination = statePtr->pc + offsetField + 1;

    if(destination < 0 || destination >= NUMMEMORY) {
        printf("beq: memory destination out of range\n");
        return 1;
    }

    if(statePtr->reg[regA] == statePtr->reg[regB]) {
        statePtr->pc = destination;
    }

    else statePtr->pc += 1;
    return 0;
}

int func_jalr(stateType *statePtr, int instruction) {
    int regA = (instruction >> 19) & 0x7;
    int regB = (instruction >> 16) & 0x7;
    int destination;

    statePtr->reg[regB] = statePtr->pc + 1;
    destination = statePtr->reg[regA];

    if(destination < 0 || destination >= NUMMEMORY) {
        printf("jalr: memory destination out of range\n");
        return 1;
    }

    statePtr->pc = destination;
    return 0;
}