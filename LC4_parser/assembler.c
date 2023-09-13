/***************************************************************************
 * file name   : assembler.c                                               *
 * author      : tjf & you                                                 *
 * description : This program will assemble a .ASM file into a .OBJ file   *
 *               This program will use the "asm_parser" library to achieve *
 *			     its functionality.										   * 
 *                                                                         *
 ***************************************************************************
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "asm_parser.h"

int main(int argc, char** argv) 
{

	char* filename = NULL ;					// name of ASM file
	char  program [ROWS][COLS] ; 			// ASM file line-by-line
	char  program_bin_str [ROWS][17] ; 		// instructions converted to a binary string
	unsigned short int program_bin [ROWS] ; // instructions in binary (HEX)
	unsigned short int addresses[ROWS];		// holds addresses for .ADDR
	unsigned short int addressRows[ROWS];	// holds which rows change addresses
	unsigned short int codeRows[ROWS];		// holds which rows have .CODE
	unsigned short int dataRows[ROWS];		// holds which rows have .DATA
	unsigned short int fillRows[ROWS];		// holds which rows have .FILL

	//array initializations
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			program[i][j] = '\0';
		}
		for (int j = 0; j < 17; j++)
		{
			program_bin_str[i][j] = '\0';
		}
		program_bin[i] = 0;
	}
	

	// handle arguments
	if (argc == 2)
	{
		filename = argv[1];
	} else
	{
		printf("error1: usage: ./assembler <assembly_file.asm>\n");
		return 1;
	}

	// run read_asm_file, and if it returns 2, print error
	if (read_asm_file(filename, program, addresses, addressRows, codeRows, dataRows, program_bin, fillRows) == 2)
	{
		printf("error2: read_asm_file() failed\n");
		return 2;
	}
	
	// parse instructions for all ROWS
	int parseCheck;
	for (int i = 0; i < ROWS; i++)
	{
		parseCheck = parse_instruction(program[i], program_bin_str[i]);

		switch (parseCheck)
		{
			case 3:
				printf("error3: parse_instruction() failed\n");
				return 3;
			case 4:
				printf("error4: parse_add() failed\n");
				return 4;
			case 5:
				printf("error5: parse_reg() failed\n");
				return 5;
			default:
				break;
		}

		// if there's no fill .FILL, there's something to convert to bin
		if (fillRows[i] == 0)
		{
			program_bin[i] = str_to_bin(program_bin_str[i]);
		}
	}

	// nothing has failed yet, so the final return is calling write_obj_file
	return write_obj_file(filename, program_bin, addresses, addressRows, codeRows, dataRows);
}
