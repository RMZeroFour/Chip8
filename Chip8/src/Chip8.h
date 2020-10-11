#pragma once
#include <string.h>
#include <functional>

#define ARRAYLEN(x) (sizeof(x) / sizeof(*x))

typedef unsigned char byte;
typedef unsigned short word;

const int PROGRAM_START = 512;

struct Opcode;

class Chip8
{
public:
	byte Memory[4096];
	word ProgramCounter;
	
	byte Registers[16]; 
	word IndexRegister;
	
	bool Graphics[64 * 32];
	bool Keyboard[16];
	
	byte DelayTimer;
	byte SoundTimer;

	byte StackPointer; 
	word Stack[16];

public:
	Chip8();

	void LoadRom(byte* code, int len);
	void UnloadRom();

	void ClockCycle();
	
private:
	void ResetCpu();
};