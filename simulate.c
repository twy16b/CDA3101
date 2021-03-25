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

int field0(int instruction) { return( (instruction>>19) & 0x7); } //RegA

int field1(int instruction) { return( (instruction>>16) & 0x7); } //RegB

int field2(int instruction) { return(instruction & 0xFFFF); } //Offsetfield

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
int destinationRegister(int instr);

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
        statePtr->dataMem[i] = readInt;
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

        /* --------------------- ID stage --------------------- */

        newState.pc = state.pc + 1;
        newState.IDEX.instr = state.IFID.instr;
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
        newState.IDEX.readRegA = state.reg[field0(state.IFID.instr)];
        newState.IDEX.readRegB = state.reg[field1(state.IFID.instr)];
        newState.IDEX.offset = convertNum(field2(state.IFID.instr));

        if(opcode(state.IDEX.instr) == LW) {
            if( (field1(state.IDEX.instr) == field0(state.IFID.instr)) ||
                (field1(state.IDEX.instr) == field1(state.IFID.instr) && opcode(state.IFID.instr) != LW) ) {
                    newState.IFID.instr = state.IFID.instr;
                    newState.IFID.pcPlus1 = state.IFID.pcPlus1;
                    newState.pc = state.pc;
                    newState.IDEX.instr = NOOPINSTRUCTION;
                    newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
                    newState.IDEX.readRegA = 0;
                    newState.IDEX.readRegB = 0;
                    newState.IDEX.offset = 0;
                }
        }
        
        /* --------------------- EX stage --------------------- */

        newState.EXMEM.instr = state.IDEX.instr;
        newState.EXMEM.branchTarget = state.IDEX.pcPlus1 + state.IDEX.offset;

        int topALU, botALU;
        
        if(field0(state.IDEX.instr) != 0) {
            if      (field0(state.IDEX.instr) == destinationRegister(state.EXMEM.instr)) topALU = state.EXMEM.aluResult;
            else if (field0(state.IDEX.instr) == destinationRegister(state.MEMWB.instr)) topALU = state.MEMWB.writeData;
            else if (field0(state.IDEX.instr) == destinationRegister(state.WBEND.instr)) topALU = state.WBEND.writeData;
            else                                                                         topALU = state.IDEX.readRegA;
        }
        else topALU = 0;

        if(field1(state.IDEX.instr) != 0) {
            if      (field1(state.IDEX.instr) == destinationRegister(state.EXMEM.instr)) newState.EXMEM.readRegB = state.EXMEM.aluResult;
            else if (field1(state.IDEX.instr) == destinationRegister(state.MEMWB.instr)) newState.EXMEM.readRegB = state.MEMWB.writeData;
            else if (field1(state.IDEX.instr) == destinationRegister(state.WBEND.instr)) newState.EXMEM.readRegB = state.WBEND.writeData;
            else                                                                         newState.EXMEM.readRegB = state.IDEX.readRegB;

            if (opcode(state.IDEX.instr) == LW || opcode(state.IDEX.instr) == SW)            botALU = state.IDEX.offset;
            else {
                if      (field1(state.IDEX.instr) == destinationRegister(state.EXMEM.instr)) botALU = state.EXMEM.aluResult;
                else if (field1(state.IDEX.instr) == destinationRegister(state.MEMWB.instr)) botALU = state.MEMWB.writeData;
                else if (field1(state.IDEX.instr) == destinationRegister(state.WBEND.instr)) botALU = state.WBEND.writeData;
                else                                                                         botALU = state.IDEX.readRegB;
            }
        }
        else {
            if (opcode(state.IDEX.instr) == LW || opcode(state.IDEX.instr) == SW) botALU = state.IDEX.offset;
            else botALU = 0;
            newState.EXMEM.readRegB = 0;
        }

        printf("topALU = %d\n",topALU);
        printf("botALU = %d\n",botALU);
        printf("readRegB = %d\n",newState.EXMEM.readRegB);

        if (opcode(state.IDEX.instr) == NAND)
            newState.EXMEM.aluResult = ~(topALU & botALU);
        else newState.EXMEM.aluResult = topALU + botALU;

        /* --------------------- MEM stage --------------------- */

        newState.MEMWB.instr = state.EXMEM.instr;
        if(opcode(state.EXMEM.instr) == ADD || opcode(state.EXMEM.instr) == NAND) 
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        else if(opcode(state.EXMEM.instr) == LW)
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        else if(opcode(state.EXMEM.instr) == SW)
            newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.readRegB;

        if (opcode(state.EXMEM.instr) == BEQ && state.EXMEM.aluResult == state.EXMEM.readRegB*2) {
            newState.pc = state.EXMEM.branchTarget;
            newState.IFID.instr = NOOPINSTRUCTION;
            newState.IDEX.instr = NOOPINSTRUCTION;
            newState.EXMEM.instr = NOOPINSTRUCTION;
        }

        /* --------------------- WB stage --------------------- */

        newState.WBEND.instr = state.MEMWB.instr;
        newState.WBEND.writeData = state.MEMWB.writeData;
        if(opcode(state.MEMWB.instr) == ADD || opcode(state.MEMWB.instr) == NAND)
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        if(opcode(state.MEMWB.instr) == LW)
            newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;

        state = newState; /* this is the last statement before end of the loop.
                    It marks the end of the cycle and updates the
                    current state with the values calculated in this
                    cycle */
    }

}

int destinationRegister(int instr) {
    if(opcode(instr) == ADD || opcode(instr) == NAND) return field2(instr);
    else if (opcode(instr) == LW) return field1(instr);
    else return 0;
}