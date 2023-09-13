/***************************************************************************
 * file name   : asm_parser.c                                              *
 * author      : tjf & you                                                 *
 * description : the functions are declared in asm_parser.h                *
 *               The intention of this library is to parse a .ASM file     *
 *			        										               * 
 *                                                                         *
 ***************************************************************************
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "asm_parser.h"

/* to do - implement all the functions in asm_parser.h */

int read_asm_file(char* filename, char program [ROWS][COLS], unsigned short int addresses[ROWS], unsigned short int addressRows[ROWS], unsigned short int codeRows[ROWS], unsigned short int dataRows[ROWS], unsigned short int program_bin [ROWS], unsigned short int fillRows[ROWS])
{
    // open file and check for success
    FILE* inputFile = fopen(filename, "r");
    if(inputFile == NULL) {return 2;}
    
    // vars for directive inputs
    int addr = 0;
    int fill = 0;

    // a temp string to check for qualities before adding to program[]
    char tempStr[COLS];
    int length;

    // loop until row hits max ROWS or EOF
    int row = 0;
    for (row = 0; row < ROWS; row++)
    {
        // load string into tempStr and break if returns EOF
        if(fgets(tempStr, COLS, inputFile) == NULL) {break;}
        length = strlen(tempStr);

        // if the line is a directive, store which row it happens at and address for .ADDR
        // for .ADDR store the address in addr if it matches template
        if (sscanf(tempStr, ".ADDR %x", &addr) == 1 && addr >= 0 && addr <= 0xFFFF)
        {
            addresses[row] = addr;
            addressRows[row] = 1;
            continue;
        } else
        {
            addressRows[row] = 0;
        }

        // for .FILL store the value in fill if it matches template
        if (sscanf(tempStr, ".FILL %d", &fill) == 1 && fill >= -32768 && fill <= 32767)
        {
            program_bin[row] = fill;
            fillRows[row] = 1;
            continue;
        } else
        {
            fillRows[row] = 0;
        }

        if (!strcmp(tempStr, ".CODE\n"))
        {
            codeRows[row] = 1;
            continue;
        } else
        {
            codeRows[row] = 0;
        }

        if (!strcmp(tempStr, ".DATA\n"))
        {
            dataRows[row] = 1;
            continue;
        } else
        {
            dataRows[row] = 0;
        }

        // loop through and assign the chars in program from tempStr
        for(int i = 0; i < length - 1; i++)
        {
            // ignore comments
            if (tempStr[i] == ';')
            {
                break;
            }

            program[row][i] = tempStr[i];
        }

        program[row][length - 1] = '\0';
    }

    if (row < ROWS)
    {
        program[row][0] = '\0';
    }

    if (row == 100)
    {
        program[99][0] = '\0';
    }

    if (row == 100 && !(tempStr[0] == '\0'))
    {
        return 2;
    }

    return 0;
}

int write_obj_file(char* filename, unsigned short int program_bin[ROWS], unsigned short int addresses[ROWS], unsigned short int addressRows[ROWS], unsigned short int codeRows[ROWS], unsigned short int dataRows[ROWS])
{
    int len = strlen(filename);

    // change file extension to .obj
    filename[len - 3] = 'o';
    filename[len - 2] = 'b';
    filename[len - 1] = 'j';

    // open the file to write binary and check for NULL
    FILE* objFile = fopen(filename, "wb");
    if (objFile == NULL)
    {
        printf("error7: write_obj_file() failed\n");
        return 7;
    }

    // vars to help with managing directives
    int rows = 0;           // num rows for currecnt header
    int tempRows = 0;       // for an edgecase where there's an empty directive
    int memoryMode = -1;     // 0: .CODE, 1: .DATA, -1: whatever comes next
    int addr = 0;           // current program address
    int dirBool = 0;        // bool for if a header has been created, if not, use the default
    char header[6];         // array for header bytes

    // loop for all ROWS
    for (int i = 0; i < ROWS; i++)
    {
        // if there's a .DATA
        if (dataRows[i] == 1)
        {
            if (memoryMode != 1)
            {
                // reset rows for this header
                tempRows = rows;
                rows = 0;

                // loop through all ROWS starting at current row
                int j;
                for (j = i; j < ROWS; j++)
                {
                    // break if it runs into a .CODE
                    if (codeRows[j] == 1)
                    {
                        break;
                    }

                    // break if it runs into a .ADDR after there's been an instruction/data
                    if (rows > 0  && addressRows[j] == 1)
                    {
                        break;
                    }

                    // if there's a new .ADDR before any instruction/data it's the new addr
                    if (rows == 0 && addressRows[j] == 1)
                    {
                        addr = addresses[j];
                    }

                    // add a row for any instruction that isn't empty
                    if (program_bin[j] != '\0')
                    {
                        rows++;
                    }
                }

                // if there are .DATA rows:
                if (rows > 0)
                {
                    // a header will be created
                    dirBool = 1;
                    memoryMode = 1;
                     // update to .DATA
                    // set header accordingly
                    
                    header[0] = 0xDA;
                    header[1] = 0xDA;
                    header[2] = (addr & 0b1111111100000000) >> 8;   // upper byte
                    header[3] = (char) addr;                        // lower byte
                    header[4] = 0x00;
                    header[5] = (char) rows;

                    // addres should go foward for the number of instruction/data rows there were
                    addr += rows;

                    // write header file to .obj
                    fwrite(header, sizeof(char), 6, objFile);

                    // write out instructions/data for all covered by this header
                    for (int k = i; k < j; k++)
                    {
                        if (program_bin[k] != '\0')
                        {
                            // upper byte
                            fputc((char) ((program_bin[k] & 0b1111111100000000) >> 8), objFile);
                            
                            // lower byte
                            fputc((char) program_bin[k], objFile);
                        }
                    }
                } else
                {
                    rows = tempRows;
                }

                // move index forward to just before the end
                i = j - 1;
            }
        }

        if (codeRows[i] == 1)
        {
            if (memoryMode != 0)
            {
                // a header will be created
                dirBool = 1;

                // reset rows for this header
                rows = 0;

                // loop through all ROWS starting at current row
                int j;
                for (j = i; j < ROWS; j++)
                {
                    // break for data row or new addr row
                    if (dataRows[j] == 1)
                    {
                        break;
                    }

                    if (rows > 0  && addressRows[j] == 1)
                    {
                        break;
                    }

                    if (rows == 0 && addressRows[j] == 1)
                    {
                        addr = addresses[j];
                    }

                    if (program_bin[j] != '\0')
                    {
                        rows++;
                    }
                }

                // update memoryMode and header
                memoryMode = 0;
                header[0] = 0xCA;
                header[1] = 0xDE;
                header[2] = (addr & 0b1111111100000000) >> 8;
                header[3] = (char) addr;
                header[4] = 0x00;
                header[5] = (char) rows;

                addr += rows;

                // print header and all instruction rows
                fwrite(header, sizeof(char), 6, objFile);

                for (int k = i; k < j; k++)
                {
                    if (program_bin[k] != '\0')
                    {
                        // upper byte
                        fputc((char) ((program_bin[k] & 0b1111111100000000) >> 8), objFile);
                        
                        // lower byte
                        fputc((char) program_bin[k], objFile);
                    }
                }

                i = j - 1;
            }
        }
        
        // if there's .ADDR
        if (addressRows[i] == 1)
        {
            dirBool = 1;
            addr = addresses[i];
            rows = 0;

            // if there's been no memoryMode yet, default to .CODE
            if (memoryMode == -1)
            {
                memoryMode = 0;
            }

            // loop through all ROWS starting at current row
            int j;
            for (j = i; j < ROWS; j++)
            {
                // if it runs into any new directives which would change things, break
                if (codeRows[j] == 1 && memoryMode == 1)
                {
                    break;
                }

                if (dataRows[j] == 1 && memoryMode == 0)
                {
                    break;
                }

                if (rows > 0  && addressRows[j] == 1)
                {
                    break;
                }

                if (rows == 0 && addressRows[j] == 1)
                {
                    addr = addresses[j];
                }

                if (program_bin[j] != '\0')
                {
                    rows++;
                }
            }

            // write header and rows using appropriate header .CODE/.DATA
            if (memoryMode == 0)
            {
                header[0] = 0xCA;
                header[1] = 0xDE;
            }

            if (memoryMode == 1)
            {
                header[0] = 0xDA;
                header[1] = 0xDA;
            }
            header[2] = (addr & 0b1111111100000000) >> 8;
            header[3] = (char) addr;
            header[4] = 0x00;
            header[5] = (char) rows;

            addr += rows;

            fwrite(header, sizeof(char), 6, objFile);

            for (int k = i; k < j; k++)
            {
                if (program_bin[k] != '\0')
                {
                    // upper byte
                    fputc((char) ((program_bin[k] & 0b1111111100000000) >> 8), objFile);
                    
                    // lower byte
                    fputc((char) program_bin[k], objFile);
                }
            }

            // return back just before the last break
            i = j - 1;
        }
        
        if (program_bin[i] != '\0')
        {
            rows++;
        }
    }

    // if there were no headers printed, print in the default way w/ .CODE at .ADDR 0x0000
    if (dirBool == 0)
    {
        //char header[] = {0xCA, 0xDE, 0x00, 0x00, 0x00, (char) rows};
        header[0] = 0xCA;
        header[1] = 0xDE;
        header[2] = 0x00;
        header[3] = 0x00;
        header[4] = 0x00;
        header[5] = (char) rows;

        // writing header to objFile
        fwrite(header, sizeof(char), 6, objFile);
        
        // write out program_bin instructions to objFile
        for (int i = 0; i < ROWS; i++)
        {
            if (program_bin[i] != '\0')
            {
                // upper byte
                fputc((char) ((program_bin[i] & 0b1111111100000000) >> 8), objFile);
                
                // lower byte
                fputc((char) program_bin[i], objFile);
            }
        }
    }

    fclose(objFile);

    return 0;
}

unsigned short int str_to_bin (char* instr_bin_str)
{
    char* temp;
    return strtol(instr_bin_str, &temp, 2);
}

int parse_instruction(char* instr, char* instr_bin_str)
{
    if (instr[0] == '\0')
    {
        return 0;
    }
    
    // take in the opcode
    char* opcode;
    opcode = strtok(instr, " ");
    if (opcode == NULL) {return 3;}
    
    // got to appropriate helper parse function
    if (strcmp(opcode, "ADD") == 0)
    {
        return parse_add(instr, instr_bin_str);
    }

    if (strcmp(opcode, "MUL") == 0)
    {
        return parse_mul(instr, instr_bin_str);
    }

    if (strcmp(opcode, "SUB") == 0)
    {
        return parse_sub(instr, instr_bin_str);
    }

    if (strcmp(opcode, "DIV") == 0)
    {
        return parse_div(instr, instr_bin_str);
    }

    if (strcmp(opcode, "AND") == 0)
    {
        return parse_and(instr, instr_bin_str);
    }

    if (strcmp(opcode, "NOT") == 0)
    {
        return parse_not(instr, instr_bin_str);
    }

    if (strcmp(opcode, "OR") == 0)
    {
        return parse_or(instr, instr_bin_str);
    }

    if (strcmp(opcode, "XOR") == 0)
    {
        return parse_xor(instr, instr_bin_str);
    }

    return 3;
}


///////////PARSING HELPER FUNCTIONS//////////////

int parse_reg(char reg_num, char* instr_bin_str)
{
    // case on register number and if bad, return 5
    switch (reg_num)
    {
        case '0':
            strcat(instr_bin_str, "000");
            break;
        case '1':
            strcat(instr_bin_str, "001");
            break;
        case '2':
            strcat(instr_bin_str, "010");
            break;
        case '3':
            strcat(instr_bin_str, "011");
            break;
        case '4':
            strcat(instr_bin_str, "100");
            break;
        case '5':
            strcat(instr_bin_str, "101");
            break;
        case '6':
            strcat(instr_bin_str, "110");
            break;
        case '7':
            strcat(instr_bin_str, "111");
            break;
        default:
            return 5;
    }

    return 0;
}

int parse_add(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '0';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '0';
    instr_bin_str[12] = '0';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    if (reg[1] == 'R')
    {
        regState = parse_reg(reg[2], instr_bin_str);
        if (regState == 5) {return 5;}
    } else if(reg[1] == '#')
    {
        // get immdiate value from string
        int imm5 = atoi(&reg[2]);
        if (imm5 > 15 || imm5 < -16)
        {
            return 4;
        }
        instr_bin_str[10] = '1';

        //convert to bin for zero
        if (imm5 == 0)
        {
            instr_bin_str[11] = '0';
            instr_bin_str[12] = '0';
            instr_bin_str[13] = '0';
            instr_bin_str[14] = '0';
            instr_bin_str[15] = '0';
        }

        // convert to bin for positive
        if (imm5 > 0)
        {
            instr_bin_str[11] = '0';
            for (int i = 15; i > 11; i--)
            {
                instr_bin_str[i] = (char) (48 + (imm5 % 2));
                imm5 /= 2;
            }
        }

        // convert to bin for negative
        if (imm5 < 0)
        {
            instr_bin_str[11] = '1';

            // it's like converting 16 + imm5 with a leading 1
            imm5 += 16;
            for (int i = 15; i > 11; i--)
            {
                instr_bin_str[i] = (char) (48 + (imm5 % 2));
                imm5 /= 2;
            }
        }
    }


    instr_bin_str[16] = '\0';

    return regState;
}

int parse_mul(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '0';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 6;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '0';
    instr_bin_str[12] = '1';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[2], instr_bin_str);
    if (regState == 5) {return 5;}

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_sub(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '0';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 7;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '1';
    instr_bin_str[12] = '0';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[2], instr_bin_str);
    if (regState == 5) {return 5;}

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_div(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '0';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 8;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '1';
    instr_bin_str[12] = '1';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[2], instr_bin_str);
    if (regState == 5) {return 5;}

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_and(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '1';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 9;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '0';
    instr_bin_str[12] = '0';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    if (reg[1] == 'R')
    {
        regState = parse_reg(reg[2], instr_bin_str);
        if (regState == 5) {return 5;}
    } else if(reg[1] == '#')
    {
        // get immdiate value from string
        int imm5 = atoi(&reg[2]);
        if (imm5 > 15 || imm5 < -16)
        {
            return 9;
        }
        instr_bin_str[10] = '1';

        //convert to bin for zero
        if (imm5 == 0)
        {
            instr_bin_str[11] = '0';
            instr_bin_str[12] = '0';
            instr_bin_str[13] = '0';
            instr_bin_str[14] = '0';
            instr_bin_str[15] = '0';
        }

        // convert to bin for positive
        if (imm5 > 0)
        {
            instr_bin_str[11] = '0';
            for (int i = 15; i > 11; i--)
            {
                instr_bin_str[i] = (char) (48 + (imm5 % 2));
                imm5 /= 2;
            }
        }

        // convert to bin for negative
        if (imm5 < 0)
        {
            instr_bin_str[11] = '1';

            // it's like converting 16 + imm5 with a leading 1
            imm5 += 16;
            for (int i = 15; i > 11; i--)
            {
                instr_bin_str[i] = (char) (48 + (imm5 % 2));
                imm5 /= 2;
            }
        }
    }

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_not(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '1';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 10;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000---
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '0';
    instr_bin_str[12] = '1';
    instr_bin_str[13] = '0';
    instr_bin_str[14] = '0';
    instr_bin_str[15] = '0';
    

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_or(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '1';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 11;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '1';
    instr_bin_str[12] = '0';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[2], instr_bin_str);
    if (regState == 5) {return 5;}

    instr_bin_str[16] = '\0';

    return regState;
}

int parse_xor(char* instr, char* instr_bin_str)
{
    char* reg;
    int regState = 0;

    // 0001
    instr_bin_str[0] = '0';
    instr_bin_str[1] = '1';
    instr_bin_str[2] = '0';
    instr_bin_str[3] = '1';

    // Rd
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 12;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // Rs
    reg = strtok(NULL, ", ");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[1], instr_bin_str);
    if (regState == 5) {return 5;}

    // 000
    instr_bin_str[10] = '0';
    instr_bin_str[11] = '1';
    instr_bin_str[12] = '1';

    // Rt
    reg = strtok(NULL, "\0");
    if (reg == NULL) {return 4;}
    regState = parse_reg(reg[2], instr_bin_str);
    if (regState == 5) {return 5;}

    instr_bin_str[16] = '\0';

    return regState;
}
