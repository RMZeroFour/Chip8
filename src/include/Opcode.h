#pragma once
#include <functional>
#include <random>

#include "Chip8.h"

struct Opcode
{
private:
	std::function<void(word, Chip8&)> executor;

public:
	const char* Description;

	Opcode(const char* desc, std::function<void(word, Chip8&)> exec)
		: Description(desc), executor(exec)
	{ }

	void Execute(word code, Chip8& cpu) { return executor(code, cpu); }
};

namespace Opcodes
{
	const Opcode& Match(word code);

	const Opcode Nop("NOP: No operation", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += 2;
		});

	const Opcode Op00E0("CLS (00E0): Clear the display", [](word code, Chip8& cpu)
		{
			memset(cpu.Graphics, 0, ARRAYLEN(cpu.Graphics));
			cpu.ProgramCounter += 2;
		});

	const Opcode Op00EE("RET (00EE): Return from subroutine", [](word code, Chip8& cpu)
		{
			cpu.StackPointer--;
			cpu.ProgramCounter = cpu.Stack[cpu.StackPointer] + 2;
		});

	const Opcode Op1nnn("JP addr (01nn): Jump to address nnn", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter = (code & 0x0FFF);
		});

	const Opcode Op2nnn("CALL addr (02nn): Call subroutine at nnn", [](word code, Chip8& cpu)
		{
			cpu.Stack[cpu.StackPointer] = cpu.ProgramCounter;
			cpu.StackPointer++;
			cpu.ProgramCounter = (code & 0x0FFF);
		});

	const Opcode Op3xkk("SE Vx, kk (3xkk): Skip next if Vx == kk", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += (cpu.Registers[(code & 0x0F00) >> 8] == (code & 0x00FF)) ? 4 : 2;
		});

	const Opcode Op4xkk("SNE Vx, kk (4xkk): Skip next if Vx != kk", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += (cpu.Registers[(code & 0x0F00) >> 8] != (code & 0x00FF)) ? 4 : 2;
		});

	const Opcode Op5xy0("SE Vx, Vy (5xy0): Skip next if Vx == Vy", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += (cpu.Registers[(code & 0x0F00) >> 8] == cpu.Registers[(code & 0x00F0) >> 4]) ? 4 : 2;
		});

	const Opcode Op6xkk("LD Vx, kk (6xkk): Load kk into Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] = (code & 0x00FF);
			cpu.ProgramCounter += 2;
		});

	const Opcode Op7xkk("ADD Vx, kk (7xkk): Set Vx to Vx + kk", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] += (code & 0x00FF);
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy0("LD Vx, Vy (8xy0): Load Vy into Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] = cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy1("OR Vx, Vy (8xy1): Set Vx to Vx OR Vy", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] |= cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy2("AND Vx, Vy (8xy2): Set Vx to Vx AND Vy", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] &= cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy3("XOR Vx, Vy (8xy3): Set Vx to Vx XOR Vy", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] ^= cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy4("ADD Vx, Vy (8xy4): Set Vx to Vx + Vy", [](word code, Chip8& cpu)
		{
			cpu.Registers[0xF] = ((int)(cpu.Registers[(code & 0x0F00) >> 8]) + cpu.Registers[(code & 0x00F0) >> 4] > 0xFF) ? 1 : 0;
			cpu.Registers[(code & 0x0F00) >> 8] += cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy5("SUB Vx, Vy (8xy5): Set Vx to Vx - Vy", [](word code, Chip8& cpu)
		{
			cpu.Registers[0xF] = ((int)(cpu.Registers[(code & 0x0F00) >> 8]) - cpu.Registers[(code & 0x00F0) >> 4] < 0) ? 0 : 1;
			cpu.Registers[(code & 0x0F00) >> 8] -= cpu.Registers[(code & 0x00F0) >> 4];
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy6("SHR Vx (8xy6): Right shift Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[0xF] = cpu.Registers[(code & 0x0F00) >> 8] & 0x1;
			cpu.Registers[(code & 0x0F00) >> 8] >>= 1;
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xy7("SUB Vx, Vy (8xy7): Set Vx to Vy - Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[0xF] = (cpu.Registers[(code & 0x00F0) >> 4]) > (cpu.Registers[(code & 0x0F00) >> 8]) ? 1 : 0;
			cpu.Registers[(code & 0x0F00) >> 8] = (cpu.Registers[(code & 0x00F0) >> 4]) - (cpu.Registers[(code & 0x0F00) >> 8]);
			cpu.ProgramCounter += 2;
		});

	const Opcode Op8xyE("SHL Vx (8xyE): Left shift Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[0xF] = (cpu.Registers[(code & 0x0F00) >> 8] & 0x80) >> 0x7;
			cpu.Registers[(code & 0x0F00) >> 8] <<= 1;
			cpu.ProgramCounter += 2;
		});

	const Opcode Op9xy0("SNE Vx, Vy (9xy0): Skip next if Vx != Vy", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += (cpu.Registers[(code & 0x0F00) >> 8] != cpu.Registers[(code & 0x00F0) >> 4]) ? 4 : 2;
		});

	const Opcode OpAnnn("LD I, nnn (Annn): Load nnn into I", [](word code, Chip8& cpu)
		{
			cpu.IndexRegister = (code & 0x0FFF);
			cpu.ProgramCounter += 2;
		});

	const Opcode OpBnnn("JP V0, addr (Bnnn): Jump to address V0 + nnn", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter = (code & 0x0FFF) + cpu.Registers[0];
		});

	const Opcode OpCxkk("RND Vx, kk (Cxkk): Set Vx to Random AND kk", [](word code, Chip8& cpu)
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(0, 255);

			cpu.Registers[(code & 0x0F00) >> 8] = dist(mt) & (code & 0x00FF);
			cpu.ProgramCounter += 2;
		});

	const Opcode OpDxyn("DRW Vx, Vy, n (Dxyn): Draw n bytes from address I at (Vx,Vy)", [](word code, Chip8& cpu)
		{
			byte x = cpu.Registers[(code & 0x0F00) >> 8] % 64;
			byte y = cpu.Registers[(code & 0x00F0) >> 4] % 32;
			cpu.Registers[0xF] = 0;

			for (int row = 0; row < (code & 0x000F); row++)
			{
				byte spriteRow = cpu.Memory[cpu.IndexRegister + row];

				for (int col = 0; col < 8; col++)
				{
					byte spritePixel = spriteRow & (0x80 >> col);
					bool* screenPixel = &cpu.Graphics[(y + row) * 64 + (x + col)];

					if (spritePixel)
					{
						if (*screenPixel)
							cpu.Registers[0xF] = 1;

						*screenPixel = !*screenPixel;
					}
				}
			}

			cpu.ProgramCounter += 2;
		});

	const Opcode OpEx9E("SKP Vx (Ex9E): Skip next if K is pressed", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += (cpu.Keyboard[cpu.Registers[(code & 0x0F00) >> 8]]) ? 4 : 2;
		});

	const Opcode OpExA1("SKNP Vx (ExA1): Skip next if K is not pressed", [](word code, Chip8& cpu)
		{
			cpu.ProgramCounter += !(cpu.Keyboard[cpu.Registers[(code & 0x0F00) >> 8]]) ? 4 : 2;
		});

	const Opcode OpFx07("LD Vx, DT (Fx07): Load DT into Vx", [](word code, Chip8& cpu)
		{
			cpu.Registers[(code & 0x0F00) >> 8] = cpu.DelayTimer;
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx0A("LD Vx, K (Fx0A): Wait and Load K into Vx", [](word code, Chip8& cpu)
		{
			for (int k = 0; k < 16; k++)
			{
				if (cpu.Keyboard[k])
				{
					cpu.Registers[(code & 0x0F00) >> 8] = k;
					cpu.ProgramCounter += 2;
				}
			}
		});

	const Opcode OpFx15("LD DT, Vx (Fx15): Load Vx into DT", [](word code, Chip8& cpu)
		{
			cpu.DelayTimer = cpu.Registers[(code & 0x0F00) >> 8];
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx18("LD ST, Vx (Fx18): Load Vx into ST", [](word code, Chip8& cpu)
		{
			cpu.SoundTimer = cpu.Registers[(code & 0x0F00) >> 8];
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx1E("ADD I, Vx (Fx1E): Set I to I + Vx", [](word code, Chip8& cpu)
		{
			cpu.IndexRegister += cpu.Registers[(code & 0x0F00) >> 8];
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx29("LD I, F (Fx29): Set I to address of char F", [](word code, Chip8& cpu)
		{
			cpu.IndexRegister = 80 + (5 * cpu.Registers[(code & 0x0F00) >> 8]);
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx33("LD [I], BCD (Fx33): Set memory at I to BCD of Vx", [](word code, Chip8& cpu)
		{
			cpu.Memory[cpu.IndexRegister + 2] = cpu.Registers[(code & 0x0F00) >> 8] % 10;
			cpu.Memory[cpu.IndexRegister + 1] = (cpu.Registers[(code & 0x0F00) >> 8] / 10) % 10;
			cpu.Memory[cpu.IndexRegister + 0] = cpu.Registers[(code & 0x0F00) >> 8] / 100;
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx55("LD [I], V (Fx55): Store V0-Vx at address I", [](word code, Chip8& cpu)
		{
			for (uint8_t i = 0; i <= ((code & 0x0F00) >> 8); i++)
				cpu.Memory[cpu.IndexRegister + i] = cpu.Registers[i];
			cpu.ProgramCounter += 2;
		});

	const Opcode OpFx65("LD V, [I] (Fx65): Read memory at I into V0-Vx", [](word code, Chip8& cpu)
		{
			for (uint8_t i = 0; i <= ((code & 0x0F00) >> 8); i++)
				cpu.Registers[i] = cpu.Memory[cpu.IndexRegister + i];
			cpu.ProgramCounter += 2;
		});
}