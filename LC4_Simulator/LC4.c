/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>


/*
 * Return rs from input MUX value
 */
unsigned char getRS(MachineState* CPU)
{
    unsigned short int instr = CPU->memory[CPU->PC];
    switch (CPU->rsMux_CTL)
    {
        case 0:
            return (instr >> 6) & 0b0000000000000111;
        case 1:
            return 7;
        case 2:
            return (instr >> 9) & 0b0000000000000111;
        default:
            break;
    }

    return 8;
}

/*
 * Return rd from input MUX value
 */
unsigned char getRD(MachineState* CPU)
{
    unsigned short int instr = CPU->memory[CPU->PC];
    switch (CPU->rdMux_CTL)
    {
        case 0:
            return (instr >> 9) & 0b0000000000000111;
        case 1:
            return 7;
        default:
            break;
    }

    return 8;
}

/*
 * Return rt from input MUX value
 */
unsigned char getRT(MachineState* CPU)
{
    unsigned short int instr = CPU->memory[CPU->PC];
    switch (CPU->rtMux_CTL)
    {
        case 0:
            return instr & 0b0000000000000111;
        case 1:
            return (instr >> 9) & 0b0000000000000111;
        default:
            break;
    }

    return 8;
}

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
    CPU->PC = 0x8200;
    CPU->PSR = 0b1000000000000010;
    for (int i = 0; i < 8; i++)
    {
        CPU->R[i] = 0;
    }

    for (int i = 0; i < 65536; i++)
    {
        CPU->memory[i] = 0;
    }

    ClearSignals(CPU);
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
    // convert instruction to binary string
    char instrBin[17];
    unsigned short int instr = CPU->memory[CPU->PC];
    for (int i = 15; i >= 0; i--)
    {
        instrBin[i] = (char) (instr % 2 + '0');
        instr /= 2;
    }
    instrBin[16] = '\0';

    // printf("Instr: %s\n", instrBin);

    // write out 1-3
    fprintf(output, "%04X %s %X ", CPU->PC, instrBin, CPU->regFile_WE);

    // write 4 & 5
    if (CPU->regFile_WE)
    {
        instr = (CPU->memory[CPU->PC] >> 12) & 0b0000000000001111;
        // if isntr modifies R7, use 7, otheriwse get rd from instruction
        if (instr == 0b0100 || instr == 0b1111 || instr == 0b1000)
        {
            instr = 7;
        } else {instr = (CPU->memory[CPU->PC] >> 9) & 0b0000000000000111;}

        fprintf(output, "%X %04X ", instr, CPU->regInputVal);
    } else
    {
        fprintf(output, "0 0000 ");
    }

    // write 6
    fprintf(output, "%X ", CPU->NZP_WE);

    // write 7
    if (CPU->NZP_WE)
    {
        fprintf(output, "%X ", CPU->NZPVal);
    } else
    {
        fprintf(output, "0 ");
    }

    // write 8
    fprintf(output, "%X ", CPU->DATA_WE);

    // write 9 & 10
    if (CPU->DATA_WE || ((CPU->memory[CPU->PC] >> 12) & 0b0000000000001111) == 0b0110)
    {
        fprintf(output, "%04X %04X\n", CPU->dmemAddr, CPU->dmemValue);
    } else
    {
        fprintf(output, "0000 0000\n");
    }
}


/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{
    unsigned short int opcode = CPU->memory[CPU->PC];

    // NOP
    if (opcode < 512)
    {
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 0;
        CPU->NZP_WE = 0;
        CPU->DATA_WE = 0;

        WriteOut(CPU, output);
        CPU->PC++;
        return 0;
    }

    opcode = (opcode >> 12) & 0b0000000000001111;

    switch (opcode)
    {
        case 0b0000:
            BranchOp(CPU, output);
            break;
        case 0b0001:
            ArithmeticOp(CPU, output);
            break;
        case 0b0010:
            ComparativeOp(CPU, output);
            break;
        case 0b0101:
            LogicalOp(CPU, output);
            break;
        case 0b1100:
            JumpOp(CPU, output);
            break;
        case 0b0100:
            JSROp(CPU, output);
            break;
        case 0b1010:
            ShiftModOp(CPU, output);
            break;
        default:
            break;
    }

    //LDR
    if (opcode == 0b0110)
    {
        // adjust control signals
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 1;
        CPU->NZP_WE = 1;
        CPU->DATA_WE = 0;

        unsigned char rs = getRS(CPU);
        unsigned char rd = getRD(CPU);

        // create SEXT(IMM6)
        short int IMM6 = CPU->memory[CPU->PC];
        if (((IMM6 >> 5) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM6 = IMM6 | 0b1111111111000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM6 = IMM6 & 0b0000000000111111;
        }

        CPU->dmemAddr = CPU->R[rs] + IMM6;

        if ((CPU->dmemAddr < 0x2000) || (CPU->dmemAddr < 0xA000 && CPU->dmemAddr > 0x7FFF))
        {
            return 1;
        }

        if ((CPU->dmemAddr >= 0x8000) && ((CPU->PSR >> 15) & 0b0000000000000001) == 0)
        {
            return 2;
        }

        CPU->regInputVal = CPU->memory[CPU->dmemAddr];
        CPU->dmemValue = CPU->regInputVal;

        CPU->R[rd] = CPU->regInputVal;

        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);
        CPU->PC++;
    }

    // STR
    if (opcode == 0b0111)
    {
        // adjust control signals
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 1;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 0;
        CPU->NZP_WE = 0;
        CPU->DATA_WE = 1;

        unsigned char rs = getRS(CPU);
        unsigned char rt = getRT(CPU);

        // create SEXT(IMM6)
        short int IMM6 = CPU->memory[CPU->PC];
        if (((IMM6 >> 5) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM6 = IMM6 | 0b1111111111000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM6 = IMM6 & 0b0000000000111111;
        }

        CPU->dmemAddr = CPU->R[rs] + IMM6;

        if ((CPU->dmemAddr < 0x2000) || (CPU->dmemAddr < 0xA000 && CPU->dmemAddr > 0x7FFF))
        {
            return 1;
        }

        if ((CPU->dmemAddr >= 0x8000) && ((CPU->PSR >> 15) & 0b0000000000000001) == 0)
        {
            return 2;
        }

        CPU->dmemValue = CPU->R[rt];
        CPU->memory[CPU->dmemAddr] = CPU->dmemValue;

        SetNZP(CPU, CPU->dmemAddr);

        WriteOut(CPU, output);
        CPU->PC++;
    }

    // RTI
    if (opcode == 0b1000)
    {
        // adjust control signals
        CPU->rsMux_CTL = 1;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 0;
        CPU->NZP_WE = 0;
        CPU->DATA_WE = 0;

        WriteOut(CPU, output);

        CPU->PC = CPU->R[7];
        SetNZP(CPU, CPU->R[7]);

        CPU->PSR = CPU->PSR & 0b0111111111111111;
    }

    // CONST
    if (opcode == 0b1001)
    {
        // adjust control signals
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 1;
        CPU->NZP_WE = 1;
        CPU->DATA_WE = 0;

        unsigned char rd = getRD(CPU);

        // create SEXT(IMM9)
        short int IMM9 = CPU->memory[CPU->PC];
        if (((IMM9 >> 8) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM9 = IMM9 | 0b1111111000000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM9 = IMM9 & 0b0000000111111111;
        }

        CPU->regInputVal = IMM9;
        CPU->R[rd] = CPU->regInputVal;

        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);
        CPU->PC++;
    }

    // HICONST
    if (opcode == 0b1101)
    {
        // adjust control signals
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 0;
        CPU->regFile_WE = 1;
        CPU->NZP_WE = 1;
        CPU->DATA_WE = 0;

        unsigned char rd = getRD(CPU);

        // create SEXT(IMM8)
        short int IMM8 = CPU->memory[CPU->PC];
        if (((IMM8 >> 7) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM8 = IMM8 | 0b1111111100000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM8 = IMM8 & 0b0000000011111111;
        }

        CPU->regInputVal = (CPU->R[rd] & 0xFF) | (IMM8 << 8);
        CPU->R[rd] = CPU->regInputVal;

        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);
        CPU->PC++;
    }

    // TRAP
    if (opcode == 0b1111)
    {
        // adjust control signals
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 0;
        CPU->rdMux_CTL = 1;
        CPU->regFile_WE = 1;
        CPU->NZP_WE = 1;
        CPU->DATA_WE = 0;

        // create UIMM8
        short int UIMM8 = CPU->memory[CPU->PC] & 0b0000000011111111;

        CPU->regInputVal = CPU->PC + 1;
        CPU->R[7] = CPU->regInputVal;
        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);

        CPU->PC = (0x8000 | UIMM8);

        CPU->PSR = CPU->PSR | 0b1000000000000000;
    }

    return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    // bitmask for NZP instruction
    unsigned short int instr = (CPU->memory[CPU->PC] >> 9) & 0b0000000000000111;

    // bit masks to get NZP values from PSR
    unsigned char N = (CPU->PSR >> 2) & 0b0000000000000001;
    unsigned char Z = (CPU->PSR >> 1) & 0b0000000000000001;
    unsigned char P = CPU->PSR & 0b0000000000000001;
    // printf("NZP: %d%d%d\n", N, Z, P);

    // create SEXT(IMM9)
    short int IMM9 = CPU->memory[CPU->PC];
    if (((IMM9 >> 8) & 0b0000000000000001) == 1)
    {
        // negative, extend the 1, bit mask rest to original
        IMM9 = IMM9 | 0b1111111000000000;
    } else
    {
        // positive, extend the 0, bit mask rest to original
        IMM9 = IMM9 & 0b0000000111111111;
    }

    // case on NZP
    switch (instr)
    {
        // return if fails check, else it passed so do PC + IMM9 (+1 comes after return)
        case 0b100:
            if (N == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        case 0b110:
            if (N == 0 && Z == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        case 0b101:
            if (N == 0 && P == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        case 0b010:
            if (Z == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        case 0b011:
            if (Z == 0 && P == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        case 0b001:
            if (P == 0) {WriteOut(CPU, output); CPU->PC++; return;}
            break;
        default:
            break;
    }

    WriteOut(CPU, output);

    CPU->PC += IMM9;
    CPU->PC++;
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;
    
    unsigned char rs = getRS(CPU);
    unsigned char rd = getRD(CPU);

    // check for ADD imm5
    if (((CPU->memory[CPU->PC] >> 5) & 0b0000000000000001) == 1)
    {
        // create SEXT(IMM5)
        short int IMM5 = CPU->memory[CPU->PC];
        if (((IMM5 >> 4) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM5 = IMM5 | 0b1111111111100000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM5 = IMM5 & 0b0000000000011111;
        }

        // rd = rs + SEXT(IMM5)
        CPU->regInputVal =  CPU->R[rs] + IMM5;
        CPU->R[rd] = CPU->regInputVal;

        // handle NZP reg
        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);
        CPU->PC++;
        return;
    }

    // get arithmetic subop
    unsigned short int instr = (CPU->memory[CPU->PC] >> 3) & 0b0000000000000111;   

    unsigned char rt = getRT(CPU);

    switch (instr)
    {
        // ADD
        case 0b000:
            // rd = rs + rt
            CPU->regInputVal =  CPU->R[rs] + CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // MUL
        case 0b001:
            // rd = rs * rt
            CPU->regInputVal =  CPU->R[rs] * CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // SUB
        case 0b010:
            // rd = rs - rt
            CPU->regInputVal =  CPU->R[rs] - CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // DIV
        case 0b011:
            // rd = rs / rt
            CPU->regInputVal =  CPU->R[rs] / CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
    }

    // handle NZP reg
    SetNZP(CPU, CPU->regInputVal);

    WriteOut(CPU, output);
    CPU->PC++;
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 2;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;

    unsigned char rs = getRS(CPU);
    unsigned char rt = getRT(CPU);

    // get subop
    unsigned short int instr = (CPU->memory[CPU->PC] >> 7) & 0b0000000000000011;

    short int IMM7 = CPU->memory[CPU->PC];
    unsigned short int UIMM7 = CPU->memory[CPU->PC];

    switch (instr)
    {
        //CMP
        case 0:
            // handle NZP reg
            if (((short int) CPU->R[rs]) > ((short int) CPU->R[rt]))
            {
                SetNZP(CPU, 1);
            } else if (((short int) CPU->R[rs]) < ((short int) CPU->R[rt]))
            {
                SetNZP(CPU, -1);
            } else
            {
                SetNZP(CPU, 0);
            }
            break;
        //CMPU
        case 1:
            // handle NZP reg
            if (CPU->R[rs] > CPU->R[rt])
            {
                SetNZP(CPU, 1);
            } else if (CPU->R[rs] < CPU->R[rt])
            {
                SetNZP(CPU, -1);
            } else
            {
                SetNZP(CPU, 0);
            }
            break;
        //CMPI
        case 2:
            // create SEXT(IMM7)
            if (((IMM7 >> 6) & 0b0000000000000001) == 1)
            {
                // negative, extend the 1, bit mask rest to original
                IMM7 = IMM7 | 0b1111111110000000;
            } else
            {
                // positive, extend the 0, bit mask rest to original
                IMM7 = IMM7 & 0b0000000001111111;
            }

            if (((short int) CPU->R[rs]) > IMM7)
            {
                SetNZP(CPU, 1);
            } else if (((short int) CPU->R[rs]) < IMM7)
            {
                SetNZP(CPU, -1);
            } else
            {
                SetNZP(CPU, 0);
            }
            break;
        //CMPIU
        case 3:
            if (CPU->R[rs] > UIMM7)
            {
                SetNZP(CPU, 1);
            } else if (CPU->R[rs] < UIMM7)
            {
                SetNZP(CPU, -1);
            } else
            {
                SetNZP(CPU, 0);
            }
            break;
    }

    WriteOut(CPU, output);
    CPU->PC++;
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;

    unsigned char rs = getRS(CPU);
    unsigned char rd = getRD(CPU);

    // check for AND imm5
    if (((CPU->memory[CPU->PC] >> 5) & 0b0000000000000001) == 1)
    {
        // create SEXT(IMM5)
        short int IMM5 = CPU->memory[CPU->PC];
        if (((IMM5 >> 4) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM5 = IMM5 | 0b1111111111100000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM5 = IMM5 & 0b0000000000011111;
        }

        // rd = rs & SEXT(IMM5)
        CPU->regInputVal =  CPU->R[rs] & IMM5;
        CPU->R[rd] = CPU->regInputVal;

        // handle NZP reg
        SetNZP(CPU, CPU->regInputVal);

        WriteOut(CPU, output);
        CPU->PC++;
        return;
    }

    // get logical subop
    unsigned short int instr = (CPU->memory[CPU->PC] >> 3) & 0b0000000000000111;   

    unsigned char rt = getRT(CPU);

    switch (instr)
    {
        // AND
        case 0b000:
            // rd = rs & rt
            CPU->regInputVal =  CPU->R[rs] & CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // NOT
        case 0b001:
            // rd = ~rs
            CPU->regInputVal =  ~CPU->R[rs];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // OR
        case 0b010:
            // rd = rs | rt
            CPU->regInputVal =  CPU->R[rs] | CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
        // XOR
        case 0b011:
            // rd = rs ^ rt
            CPU->regInputVal =  CPU->R[rs] ^ CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
    }

    // handle NZP reg
    SetNZP(CPU, CPU->regInputVal);

    WriteOut(CPU, output);
    CPU->PC++;
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 0;
    CPU->NZP_WE = 0;
    CPU->DATA_WE = 0;

    unsigned short int instr = (CPU->memory[CPU->PC] >> 11) & 0b0000000000000001;

    WriteOut(CPU, output);

    if (instr == 0)
    {
        unsigned char rs = getRS(CPU);
        CPU->PC = CPU->R[rs];
    } else
    {
        // create SEXT(IMM11)
        short int IMM11 = CPU->memory[CPU->PC];
        if (((IMM11 >> 10) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM11 = IMM11 | 0b1111100000000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM11 = IMM11 & 0b0000011111111111;
        }

        CPU->PC += 1 + IMM11;
    }
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
    // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 1;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;

    unsigned short int instr = (CPU->memory[CPU->PC] >> 11) & 0b0000000000000001;

    CPU->regInputVal = CPU->PC + 1;
    CPU->R[7] = CPU->regInputVal;
    SetNZP(CPU, CPU->regInputVal);

    WriteOut(CPU, output);

    if (instr == 0)
    {
        CPU->PC = CPU->R[getRS(CPU)];
    } else
    {
        // create SEXT(IMM11)
        short int IMM11 = CPU->memory[CPU->PC];
        if (((IMM11 >> 10) & 0b0000000000000001) == 1)
        {
            // negative, extend the 1, bit mask rest to original
            IMM11 = IMM11 | 0b1111100000000000;
        } else
        {
            // positive, extend the 0, bit mask rest to original
            IMM11 = IMM11 & 0b0000011111111111;
        }

        CPU->PC = (CPU->PC & 0x8000) | (IMM11 << 4);
    }
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
   // adjust control signals
    CPU->rsMux_CTL = 0;
    CPU->rtMux_CTL = 0;
    CPU->rdMux_CTL = 0;
    CPU->regFile_WE = 1;
    CPU->NZP_WE = 1;
    CPU->DATA_WE = 0;

    unsigned char rs = getRS(CPU);
    unsigned char rd = getRD(CPU);
    unsigned char rt = getRT(CPU);

    // create SEXT(IMM5)
    short int UIMM4 = CPU->memory[CPU->PC] & 0b0000000000001111;

    unsigned short int instr = (CPU->memory[CPU->PC] >> 4) & 0b0000000000000011;

    switch (instr)
    {
        // SLL
        case 0:
            CPU->regInputVal = CPU->R[rs] << UIMM4;
            CPU->R[rd] = CPU->regInputVal;
            break;
        // SRA
        case 1:
            CPU->regInputVal = (short int) CPU->R[rs] >> UIMM4;
            CPU->R[rd] = CPU->regInputVal;
            break;
        // SRL
        case 2:
            CPU->regInputVal = CPU->R[rs] >> UIMM4;
            CPU->R[rd] = CPU->regInputVal;
            break;
        // MOD
        case 3:
            CPU->regInputVal = CPU->R[rs] % CPU->R[rt];
            CPU->R[rd] = CPU->regInputVal;
            break;
    }

    SetNZP(CPU, CPU->regInputVal);
    WriteOut(CPU, output);
    CPU->PC++;
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, short result)
{
    if (result > 0)
    {
        CPU->PSR = (CPU->PSR & 0b1111111111111000) | 0b0000000000000001;
        CPU->NZPVal = 1;
    } else if (result < 0)
    {
        CPU->PSR = (CPU->PSR & 0b1111111111111000) | 0b0000000000000100;
        CPU->NZPVal = 4;
    } else
    {
        CPU->PSR = (CPU->PSR & 0b1111111111111000) | 0b0000000000000010;
        CPU->NZPVal = 2;
    }
}
