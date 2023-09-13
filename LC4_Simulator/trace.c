/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// unsigned short int getWord(FILE** filename)
// {

// }

int main(int argc, char** argv)
{
    // this assignment does NOT require dynamic memory - do not use 'malloc() or free()'
    // create a local varible in main to represent the CPU and then pass it around
    // to subsequent functions using a pointer!
    
    /* notice: there are test cases for part 1 & part 2 here on codio
       they are located under folders called: p1_test_cases & p2_test_cases
    
       once you code main() to open up files, you can access the test cases by typing:
       ./trace p1_test_cases/divide.obj   - divide.obj is just an example
    
       please note part 1 test case files have an OBJ and a TXT file
       the .TXT shows you what's in the .OBJ files
    
       part 2 test case files have an OBJ, an ASM, and a TXT files
       the .OBJ is what you must read in
       the .ASM is the original assembly file that the OBJ was produced from
       the .TXT file is the expected output of your simulator for the given OBJ file
    */

    // ensure correct usage of trace
    if (argc < 3 || strcmp(&argv[1][strlen(argv[1]) - 4], ".txt") || strcmp(&argv[2][strlen(argv[2]) - 4], ".obj"))
    {
        printf("error: usage: ./trace output_filename.txt first.obj second.obj third.obj ...\n");
        return -1;
    }

    // open filename and check for NULL
    FILE* outputFile = fopen(argv[1], "w");
    if (outputFile == NULL) {return 1;}

    // make sure all .obj files are valid
    FILE* inputFile = NULL;
    for (int i = 2; i < argc; i++)
    {
        inputFile = fopen(argv[i], "rb");
        if (inputFile == NULL) {printf("error: invalid .obj files\n"); return 1;}
    }
    
    // create and reset CPU
    MachineState CPU;
    Reset(&CPU);

    // load in .obj files
    for (int i = 2; i < argc; i++)
    {
        if (ReadObjectFile(argv[i], &CPU) == 1)
        {
            printf("error: invalid .obj files\n");
            return 1;
        }
    }

    //print all of memory which isn't holding 0
    // for (int i = 0 ; i < 65536; i++)
    // {
    //     if (CPU.memory[i] != 0)
    //     {
    //         printf("Addr: %X Value: %x\n", i, CPU.memory[i]);
    //     }
    // }

    // until the last
    while (CPU.PC != 0x80FF)
    {
        // printf("PC: %x\n", CPU.PC);

        if ((CPU.PC >= 0x8000) && ((CPU.PSR >> 15) & 0b0000000000000001) == 0)
        {
            printf("ILLEGAL: Attempting to access OS MEMEMORY with privilege FALSE\n");
            return 1;
        }

        if ((CPU.PC < 0x8000 && CPU.PC > 0x1FFF) || (CPU.PC > 0x9FFF))
        {
            printf("ILLEGAL: Attempting to execute DATA MEMEMORY as PROGRAM MEMORY\n");
            return 1;
        }

        int machineStateRet = UpdateMachineState(&CPU, outputFile);

        if (machineStateRet == 1)
        {
            printf("ILLEGAL: Attempting to access PROGRAM MEMORY as DATA MEMEMORY\n");
            return 1;
        }

        if (machineStateRet == 2)
        {
            printf("ILLEGAL: Attempting to access OS MEMEMORY with privilege FALSE\n");
            return 1;
        }

        // for (int i = 0; i < 8; i++)
        // {
        //     printf("R%d: %d\n", i, (short int) CPU.R[i]);
        // }
    }

    fclose(outputFile);

    //print all of memory which isn't holding 0
    // for (int i = 0 ; i < 65536; i++)
    // {
    //     if (CPU.memory[i] != 0)
    //     {
    //         printf("Addr: %X Value: %x\n", i, CPU.memory[i]);
    //     }
    // }

    return 0;
}