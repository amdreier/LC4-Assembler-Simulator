/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"

// memory array location
unsigned short memoryAddress;

/*
 * Read an object file and modify the machine state as described in the writeup
 */
int ReadObjectFile(char* filename, MachineState* CPU)
{
  

  // open and read all .obj files into program memory
  short int byteMS;
  short int byteLS;
  unsigned short int word;
  unsigned short int addr;
  unsigned short int n;       // n from .obj header
  
  FILE* inputFile = fopen(filename, "rb");
  if (inputFile == NULL) {return 1;}

  while (1)
  {
    // take in bytes little endian
    byteMS = fgetc(inputFile);
    byteLS = fgetc(inputFile);
    if (byteLS == -1) {break;}
    word = (byteMS << 8) | byteLS;

    // 0xCADE .CODE header or 0xDADA .DATA header
    if (word == 0xCADE || word == 0xDADA)
    {
      // get addr
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      if (byteLS == -1) {break;}
      addr = (byteMS << 8) | byteLS;

      // get n
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      if (byteLS == -1) {break;}
      n = (byteMS << 8) | byteLS;

      for (int i = 0; i < n; i++)
      {
        // get word
        byteMS = fgetc(inputFile);
        byteLS = fgetc(inputFile);
        if (byteLS == -1) {break;}
        word = (byteMS << 8) | byteLS;
        CPU->memory[addr] = word;
        addr++;
      }
    }

    //Symbol
    if (word == 0xC3B7)
    {
      // skip one word for addr
      byteLS = fgetc(inputFile);
      byteLS = fgetc(inputFile);

      // get n
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      if (byteLS == -1) {break;}
      n = (byteMS << 8) | byteLS;

      for (int i = 0; i < n; i++)
      {
        // move forwards in file n bytes
        byteMS = fgetc(inputFile);
        if (byteMS == -1) {break;}
      }
    }

    // File name
    if (word == 0xF17E)
    {
      // get n
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      if (byteLS == -1) {break;}
      n = (byteMS << 8) | byteLS;

      for (int i = 0; i < n; i++)
      {
        // move forwards in file n bytes
        byteMS = fgetc(inputFile);
        if (byteMS == -1) {break;}
      }
    }

    // Line number
    if (word == 0x715E)
    {
      // move forward 3 words
      byteLS = fgetc(inputFile);
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      byteMS = fgetc(inputFile);
      byteLS = fgetc(inputFile);
      byteMS = fgetc(inputFile);
      if (byteMS == -1) {break;}
    } 
  }

  fclose(inputFile);
  return 0;
}
