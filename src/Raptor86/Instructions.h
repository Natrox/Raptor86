// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

namespace Raptor
{
	namespace r86
	{
		// List of instructions currently added to the virtual machine.
		// Some operations may be unimplemented. See DOCUMENTATION.
		namespace Instructions
		{
			enum Instruction
			{
				ASM_ADD,
				ASM_AND,
				ASM_ASYNCK,
				ASM_CALL,
				ASM_CMP,
				ASM_DEC,
				ASM_DIV,
				ASM_EPILOG,
				ASM_FABS,
				ASM_FADD,
				ASM_FATAN,
				ASM_FCHS,
				ASM_FCOM,
				ASM_FCOS,
				ASM_FDIV,
				ASM_FMUL,
				ASM_FPOW,
				ASM_FPRINT,
				ASM_FSIN,
				ASM_FSQRT,
				ASM_FSUB,
				ASM_FTAN,
				ASM_FTOI,
				ASM_GETCH,
				ASM_IAND,
				ASM_IDIV,
				ASM_IMOD,
				ASM_IMUL,
				ASM_INC,
				ASM_INT,
				ASM_IOR,
				ASM_IPRINT,
				ASM_IXOR,
				ASM_JA,
				ASM_JAE,
				ASM_JB,
				ASM_JBE,
				ASM_JE,
				ASM_JG,
				ASM_JGE,
				ASM_JL,
				ASM_JLE,
				ASM_JMP,
				ASM_JNE,
				ASM_JNO,
				ASM_JNS,
				ASM_JO,
				ASM_JS,
				ASM_JZ,
				ASM_LEA,
				ASM_MOD,
				ASM_MOV,
				ASM_MUL,
				ASM_NEG,
				ASM_NOP,
				ASM_NOT,
				ASM_OR,
				ASM_POP,
				ASM_PROLOG,
				ASM_PUSH,
				ASM_RCLR,
				ASM_RET,
				ASM_RGET,
				ASM_RPLOT,
				ASM_RPOS,
				ASM_SAL,
				ASM_SAR,
				ASM_SHL,
				ASM_SHR,
				ASM_SIF,
				ASM_SLEEP,
				ASM_SUB,
				ASM_TEST,
				ASM_TIME,
				ASM_UIF,
				ASM_UPRINT,
				ASM_XADD,
				ASM_XCHG,
				ASM_XOR
			};

			static const unsigned int NUMBER_OF_INSTRUCTIONS = ASM_XOR + 1;
		};
	};
};