#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------Provided in Project2.txt------DO NOT CHANGE------------

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR will not implemented for Project 2 */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDStruct {
    int instr;
    int pcPlus1;
} IFIDType;

typedef struct IDEXStruct {
    int instr;
    int pcPlus1;
    int readRegA;
    int readRegB;
    int offset;
} IDEXType;

typedef struct EXMEMStruct {
    int instr;
    int branchTarget;
    int aluResult;
    int readRegB;
} EXMEMType;

typedef struct MEMWBStruct {
    int instr;
    int writeData;
} MEMWBType;

typedef struct WBENDStruct {
    int instr;
    int writeData;
} WBENDType;

typedef struct stateStruct {
    int pc;
    int instrMem[NUMMEMORY];
    int dataMem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
    IFIDType IFID;
    IDEXType IDEX;
    EXMEMType EXMEM;
    MEMWBType MEMWB;
    WBENDType WBEND;
    int cycles; /* number of cycles run so far */
} stateType;

int field0(int instruction) { return( (instruction>>19) & 0x7); }

int field1(int instruction) { return( (instruction>>16) & 0x7); }

int field2(int instruction) { return(instruction & 0xFFFF); }

int opcode(int instruction) { return(instruction>>22); }

void printInstruction(int instr) {

    char opcodeString[10];
    if (opcode(instr) == ADD) {
	strcpy(opcodeString, "add");
    } else if (opcode(instr) == NAND) {
	strcpy(opcodeString, "nand");
    } else if (opcode(instr) == LW) {
	strcpy(opcodeString, "lw");
    } else if (opcode(instr) == SW) {
	strcpy(opcodeString, "sw");
    } else if (opcode(instr) == BEQ) {
	strcpy(opcodeString, "beq");
    } else if (opcode(instr) == JALR) {
	strcpy(opcodeString, "jalr");
    } else if (opcode(instr) == HALT) {
	strcpy(opcodeString, "halt");
    } else if (opcode(instr) == NOOP) {
	strcpy(opcodeString, "noop");
    } else {
	strcpy(opcodeString, "data");
    }

    printf("%s %d %d %d\n", opcodeString, field0(instr), field1(instr),
	field2(instr));

}

void printState(stateType *statePtr) {

    int i;
    printf("\n@@@\nstate before cycle %d starts\n", statePtr->cycles);
    printf("\tpc %d\n", statePtr->pc);

    printf("\tdata memory:\n");
	for (i=0; i<statePtr->numMemory; i++) {
	    printf("\t\tdataMem[ %d ] %d\n", i, statePtr->dataMem[i]);
	}
    printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++) {
	    printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	}
    printf("\tIFID:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IFID.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IFID.pcPlus1);
    printf("\tIDEX:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->IDEX.instr);
	printf("\t\tpcPlus1 %d\n", statePtr->IDEX.pcPlus1);
	printf("\t\treadRegA %d\n", statePtr->IDEX.readRegA);
	printf("\t\treadRegB %d\n", statePtr->IDEX.readRegB);
	printf("\t\toffset %d\n", statePtr->IDEX.offset);
    printf("\tEXMEM:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->EXMEM.instr);
	printf("\t\tbranchTarget %d\n", statePtr->EXMEM.branchTarget);
	printf("\t\taluResult %d\n", statePtr->EXMEM.aluResult);
	printf("\t\treadRegB %d\n", statePtr->EXMEM.readRegB);
    printf("\tMEMWB:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->MEMWB.instr);
	printf("\t\twriteData %d\n", statePtr->MEMWB.writeData);
    printf("\tWBEND:\n");
	printf("\t\tinstruction ");
	printInstruction(statePtr->WBEND.instr);
	printf("\t\twriteData %d\n", statePtr->WBEND.writeData);

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

void run(stateType state);
void queueDataHazard(int hazards[], int instr);
int checkDataHazard(int hazards[], int instr);

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

    //Initialize struct to 0 and pipeline registers to NOOP
    statePtr = calloc(1, sizeof(stateType));
    statePtr->IFID.instr = NOOPINSTRUCTION;
    statePtr->IDEX.instr = NOOPINSTRUCTION;
    statePtr->EXMEM.instr = NOOPINSTRUCTION;
    statePtr->MEMWB.instr = NOOPINSTRUCTION;
    statePtr->WBEND.instr = NOOPINSTRUCTION;

    //Read machine code file into memory
    int readInt;
    fscanf(inFilePtr, "%d", &readInt);
    for (int i = 0; !feof(inFilePtr) && i < NUMMEMORY; ++i) {
        statePtr->instrMem[i] = readInt;
        statePtr->numMemory += 1;
        fscanf(inFilePtr, "%d", &readInt);
    }
    printf("\n");

    //Close file
    fclose(inFilePtr);

    //Execute instructions loop
    run(*statePtr);

    //Free up state pointer
    free(statePtr);

    return 0;
}

void run(stateType state) {

    stateType newState;
    int detectDataHazards[3] = {-1,-1,-1};
    int dataHazardPipeline[3] = {-1,-1,-1};

    for(int i = 0; i < state.numMemory; ++i) {
        printf("memory[%d]=%d\n",i,state.instrMem[i]);
    }
    printf("%d memory words\n",state.numMemory);
    printf("\tinstruction memory:\n");
    for(int i = 0; i < state.numMemory; ++i) {
        printf("\t\tinstrMem[ %d ] ", i);
        printInstruction(state.instrMem[i]);
    }

    while (1) {

        printState(&state);

        /* check for halt */
        if (opcode(state.MEMWB.instr) == HALT) {
            printf("machine halted\n");
            printf("total of %d cycles executed\n", state.cycles);
            exit(0);
        }

        newState = state;
        newState.cycles++;

        /* --------------------- IF stage --------------------- */

        newState.IFID.instr = state.instrMem[state.pc];
        newState.IFID.pcPlus1 = state.pc + 1;
        //newState.pc = state.pc + 1;

        /* --------------------- ID stage --------------------- */

        newState.IDEX.instr = state.IFID.instr;
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
        newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
        newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
        newState.IDEX.offset = convertNum(field2(state.IFID.instr));
        checkDataHazard(detectDataHazards, state.IFID.instr);
        //queueDataHazard(detectDataHazards, state.IFID.instr);
        
        /* --------------------- EX stage --------------------- */

        newState.EXMEM.instr = state.IDEX.instr;
        newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;
        newState.EXMEM.readRegB = state.IDEX.readRegB;

        int topALU, botALU, hazardA, hazardB;
        switch(opcode(state.IDEX.instr)) {
            case ADD:
                topALU = state.IDEX.readRegA;
                botALU = state.IDEX.readRegB;
                hazardA = checkDataHazard(detectDataHazards, field0(state.IDEX.instr));
                hazardB = checkDataHazard(detectDataHazards, field1(state.IDEX.instr));
                switch(hazardA) {
                    case -1:
                        break;
                    case 0:
                        topALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        topALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        topALU = state.WBEND.writeData;
                        break;
                }
                switch(hazardB) {
                    case -1:
                        break;
                    case 0:
                        botALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        botALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        botALU = state.WBEND.writeData;
                        break;
                }
                newState.EXMEM.aluResult = topALU + botALU;
                break;
            case NAND:
                topALU = state.IDEX.readRegA;
                botALU = state.IDEX.readRegB;
                hazardA = checkDataHazard(detectDataHazards, field0(state.IDEX.instr));
                hazardB = checkDataHazard(detectDataHazards, field1(state.IDEX.instr));
                switch(hazardA) {
                    case -1:
                        break;
                    case 0:
                        topALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        topALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        topALU = state.WBEND.writeData;
                        break;
                }
                switch(hazardB) {
                    case -1:
                        break;
                    case 0:
                        botALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        botALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        botALU = state.WBEND.writeData;
                        break;
                }
                newState.EXMEM.aluResult = ~(topALU & botALU);
                break;
            case LW:
                topALU = state.IDEX.readRegA;
                botALU = state.IDEX.offset;
                hazardA = checkDataHazard(detectDataHazards, field0(state.IDEX.instr));
                switch(hazardA) {
                    case -1:
                        break;
                    case 0:
                        topALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        topALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        topALU = state.WBEND.writeData;
                        break;
                }
                newState.EXMEM.aluResult = topALU + botALU;
                break;
            case SW:
                topALU = state.IDEX.readRegA;
                botALU = state.IDEX.offset;
                hazardA = checkDataHazard(detectDataHazards, field0(state.IDEX.instr));
                hazardB = checkDataHazard(detectDataHazards, field1(state.IDEX.instr));
                switch(hazardA) {
                    case -1:
                        break;
                    case 0:
                        topALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        topALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        topALU = state.WBEND.writeData;
                        break;
                }
                switch(hazardB) {
                    case -1:
                        break;
                    case 0:
                        botALU = state.EXMEM.aluResult;
                        break;
                    case 1:
                        botALU = state.MEMWB.writeData;
                        break;
                    case 2:
                        botALU = state.WBEND.writeData;
                        break;
                }
                newState.EXMEM.aluResult = topALU + botALU;
                break;
            case BEQ:
                //TODO: Check data hazards
                //if(state.IDEX.readRegA == state.IDEX.readRegB) newState.pc = state.IDEX.pcPlus1 + state.IDEX.offset;
                //else newState.pc = state.pc + 1;
                break;
            case HALT:
                //newState.pc = state.pc + 1;
                break;
            case NOOP:
                //newState.pc = state.pc + 1;
                break;

        }

        switch(opcode(state.IDEX.instr)) {
            case ADD:
                queueDataHazard(detectDataHazards, field2(state.IDEX.instr));
                break;
            case NAND:
                queueDataHazard(detectDataHazards, field2(state.IDEX.instr));
                break;
            case LW:
                queueDataHazard(detectDataHazards, field1(state.IDEX.instr));
                break;
            case SW:
                queueDataHazard(detectDataHazards, 0);
                break;
            case BEQ:
                queueDataHazard(detectDataHazards, 0);
                break;
            case HALT:
                queueDataHazard(detectDataHazards, 0);
                break;
            case NOOP:
                queueDataHazard(detectDataHazards, 0);
                break;
        }

        /* --------------------- MEM stage --------------------- */

        newState.MEMWB.instr = state.EXMEM.instr;

        switch(opcode(state.EXMEM.instr)) {
            case ADD:
                newState.MEMWB.writeData = state.EXMEM.aluResult;
                //newState.pc = state.pc + 1;
                break;
            case NAND:
                newState.MEMWB.writeData = state.EXMEM.aluResult;
                //newState.pc = state.pc + 1;
                break;
            case LW:
                newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
                //newState.pc = state.pc + 1;
                break;
            case SW:
                newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;
                //newState.pc = state.pc + 1;
                break;
            case BEQ:
                //if(state.EXMEM.aluResult == state.EXMEM.readRegB*2) newState.pc = state.EXMEM.branchTarget;
                //else newState.pc = state.pc + 1;
                break;
        }

        /* --------------------- WB stage --------------------- */

        newState.WBEND.instr = state.MEMWB.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;
        switch(opcode(state.MEMWB.instr)) {
            case ADD:
                newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
                break;
            case NAND:
                newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
                break;
            case LW:
                newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
                break;
        }

        state = newState; /* this is the last statement before end of the loop.
                    It marks the end of the cycle and updates the
                    current state with the values calculated in this
                    cycle */
    }

}

void queuePipeline(int pipe[], int val) {
    pipe[2] = pipe[1];
    pipe[1] = pipe[0];
    pipe[0] = val;
}

//Returns -1 if no hazards found
int checkDataHazard(int hazards[], int instr) {
    int hazardType;
    for(int i = 0; i < 3; ++i) {
        if(hazards[i] == 0) continue;
        else if(hazards[i] == field0(instr)) {
            queuePipeline();
        }
        else if(hazards[i] == field1(instr) && opcode(instr) != LW)
    }
    return hazardType;
}