#include "Opcode.h"

const Opcode& Opcodes::Match(word code)
{
	switch ((code >> 12))
	{
	case 0x0:
		switch (code & 0xFF)
		{
		case 0xE0: return Opcodes::Op00E0;
		case 0xEE: return Opcodes::Op00EE;
		}
		break;

	case 0x1: return Opcodes::Op1nnn;
	case 0x2: return Opcodes::Op2nnn;
	case 0x3: return Opcodes::Op3xkk;
	case 0x4: return Opcodes::Op4xkk;
	case 0x5: return Opcodes::Op5xy0;
	case 0x6: return Opcodes::Op6xkk;
	case 0x7: return Opcodes::Op7xkk;

	case 0x8:
		switch (code & 0xF)
		{
		case 0x0: return Opcodes::Op8xy0;
		case 0x1: return Opcodes::Op8xy1;
		case 0x2: return Opcodes::Op8xy2;
		case 0x3: return Opcodes::Op8xy3;
		case 0x4: return Opcodes::Op8xy4;
		case 0x5: return Opcodes::Op8xy5;
		case 0x6: return Opcodes::Op8xy6;
		case 0x7: return Opcodes::Op8xy7;
		case 0xE: return Opcodes::Op8xyE;
		}
		break;

	case 0x9: return Opcodes::Op9xy0;
	case 0xA: return Opcodes::OpAnnn;
	case 0xB: return Opcodes::OpBnnn;
	case 0xC: return Opcodes::OpCxkk;
	case 0xD: return Opcodes::OpDxyn;

	case 0xE:
		switch (code & 0xFF)
		{
		case 0xA1: return Opcodes::OpExA1;
		case 0x9E: return Opcodes::OpEx9E;
		}
		break;

	case 0xF:
		switch (code & 0xFF)
		{
		case 0x07: return Opcodes::OpFx07;
		case 0x0A: return Opcodes::OpFx0A;
		case 0x15: return Opcodes::OpFx15;
		case 0x18: return Opcodes::OpFx18;
		case 0x1E: return Opcodes::OpFx1E;
		case 0x29: return Opcodes::OpFx29;
		case 0x33: return Opcodes::OpFx33;
		case 0x55: return Opcodes::OpFx55;
		case 0x65: return Opcodes::OpFx65;
		}
		break;
	}

	return Opcodes::Nop;
}
