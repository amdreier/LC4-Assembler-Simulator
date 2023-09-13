# LC4 Assembler/Simulator
This is a final project for a class written in C. The assembly parser takes a version of assembly language with 29 instructions and four directives and translates it into an executable .obj file which can then be fed into and “run” by the simulator program, producing a trace file of the program's execution.
The project allows for use of data memory and TRAP functions, allowing it to simulate device “input” and “output” and calls to an operating system, which can also be written for the simulator in assembly.

These programs can be downloaded and compiled using their makefiles (in the each respective directory) with "make all".
Sample code for the parser and simulator can be found in the Test Cases directory, with the .asm files for the parser, the .obj files as the expected output for the parser as well as input for the simulator, and the .txt files for the expected output of the simulator
Using the dif command should return no differneces between the test files and the output files.
