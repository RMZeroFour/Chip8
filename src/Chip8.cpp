#include "Opcode.h"
#include "Chip8.h"
#include "Font.h"

Chip8::Chip8()
{
	ResetCpu();
}

void Chip8::LoadRom(byte* code, int len)
{
	ResetCpu();
	memcpy(Memory + PROGRAM_START, code, len);
}

void Chip8::UnloadRom()
{
	ResetCpu();
}

void Chip8::ClockCycle()
{
	word instruction = Memory[ProgramCounter] << 8 | Memory[ProgramCounter + 1];
	Opcode code = Opcodes::Match(instruction);
	code.Execute(instruction, *this);
	
	if (DelayTimer > 0)
		DelayTimer--;

	if (SoundTimer > 0)
		SoundTimer--;
}

void Chip8::ResetCpu()
{
	memset(Memory, 0, ARRAYLEN(Memory));
	memcpy(Memory + FONT_START, fontset, FONT_SIZE);

	memset(Registers, 0, ARRAYLEN(Registers));
	IndexRegister = 0;

	ProgramCounter = PROGRAM_START;

	StackPointer = 0;
	memset(Stack, 0, ARRAYLEN(Stack));

	DelayTimer = 0;
	SoundTimer = 0;

	memset(Graphics, 0, ARRAYLEN(Graphics));
	memset(Keyboard, 0, ARRAYLEN(Keyboard));

}