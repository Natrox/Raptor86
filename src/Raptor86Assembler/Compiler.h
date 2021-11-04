#pragma once

#include <vector>
#include <string>
#include <map>

#include "Instructions.h"

namespace States
{
	enum State
	{
		STATE_NONE,
		STATE_INSTRUCTION_SPECIFIED,
		STATE_OPERAND1_SPECIFIED,
		STATE_LEA_DATA,
		STATE_LEA_INTERNAL
	};
};

namespace ProgramLineFlags
{
	enum ProgramLineFlag
	{
		NO_FLAGS,

		OPERAND1_DEREFERENCE_REGISTER = 1,
		OPERAND2_DEREFERENCE_REGISTER = 2,
		OPERAND1_DEREFERENCE_HEAP = 4,
		OPERAND2_DEREFERENCE_HEAP = 8,
		OPERAND1_HEAP_ADDRESS = 16,
		OPERAND2_HEAP_ADDRESS = 32,
		OPERAND1_RAW_UNSIGNED_NUMBER = 64,
		OPERAND2_RAW_UNSIGNED_NUMBER = 128,
		OPERAND1_RAW_SIGNED_NUMBER = 256,
		OPERAND2_RAW_SIGNED_NUMBER = 512,
		OPERAND1_RAW_FLOAT_NUMBER = 1024,
		OPERAND2_RAW_FLOAT_NUMBER = 2048,
		OPERAND1_PROPERTY_STATIC_HEAP_SECTION = 4096,
		OPERAND2_PROPERTY_STATIC_HEAP_SECTION = 8192
	};

	static const unsigned int plf_RawRegisterFlagOp1 = 0x555;
	static const unsigned int plf_RawRegisterFlagOp2 = 0xAAA;
					  	
	static const unsigned int plf_HeapCheckFlagsOp1 = 0x14;
	static const unsigned int plf_HeapCheckFlagsOp2 = 0x28;
};

namespace InputTypes
{
	enum InputType
	{
		INPUT_UNKNOWN,

		INPUT_INSTRUCTION,

		INPUT_OPERAND_UINT,
		INPUT_OPERAND_INT,
		INPUT_OPERAND_FLOAT,

		INPUT_OPERAND_DF_GVAR,
		INPUT_OPERAND_DF_REGISTER,
		INPUT_OPERAND_DF_STACK_PTR,
		INPUT_OPERAND_DF_BASE_PTR,
		INPUT_OPERAND_DF_ADDRESS,
		INPUT_OPERAND_DF_SPECIAL_PTR,

		INPUT_OPERAND_GVAR,
		INPUT_OPERAND_REGISTER,
		INPUT_OPERAND_STACK_PTR,
		INPUT_OPERAND_BASE_PTR,
		INPUT_OPERAND_ADDRESS,
		INPUT_OPERAND_LABEL,

		INPUT_LEA_INTERNAL,
		INPUT_LEA_MATH,
		INPUT_LEA_INSTRUCTION,

		INPUT_INVALID,

		INPUT_LABEL,

		INPUT_NEW_LINE,
		INPUT_GVAR
	};
};

struct GlobalVar
{
	GlobalVar()
		: gv_Value(NULL)
		, gv_Size(0)
	{

	}

	~GlobalVar()
	{
		delete [] gv_Value;
	}

	char* gv_Value;
	unsigned int gv_Address;
	size_t gv_Size;
};

struct LabelInfo
{
	unsigned int li_Line;
	std::string li_Identifier;
};

struct ProgramLine
{
	ProgramLine( void )
	{
		pl_Opcode = 0;
		pl_Flags = 0;
		pl_Operand1 = 0;
		pl_Operand2 = 0;
		pl_Operand1Set = false;
	}

	unsigned short pl_Opcode;
	unsigned int pl_Flags;
	unsigned int pl_Operand1;
	unsigned int pl_Operand2;
	bool pl_Operand1Set;

	std::string pl_Operand1Label;
	std::string pl_Operand2Label;
};

extern std::vector< ProgramLine > g_ProgramLines;
extern std::map< std::string, InstructionInfo > g_InstructionMap;
extern std::map< std::string, LabelInfo* > g_LabelMap;
extern std::vector< LabelInfo* > g_UnboundLabel;

static unsigned int FillInstructionMap( void )
{
	unsigned int count = 0;

	g_InstructionMap[ "ADD"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "AND"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "ASYNCK" ] = InstructionInfo( count++, 0x55 );
	g_InstructionMap[ "CALL"   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "CMP"	   ] = InstructionInfo( count++, 0x3FF );
	g_InstructionMap[ "CPRINT" ] = InstructionInfo( count++, 0x115 );
	g_InstructionMap[ "DEC"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "DIV"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "EPILOG" ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "FABS"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FADD"   ] = InstructionInfo( count++, 0x83F );
	g_InstructionMap[ "FATAN"  ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FCHS"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FCOM"   ] = InstructionInfo( count++, 0xC3F );
	g_InstructionMap[ "FCOS"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FDIV"   ] = InstructionInfo( count++, 0x83F );
	g_InstructionMap[ "FMUL"   ] = InstructionInfo( count++, 0x83F );
	g_InstructionMap[ "FPOW"   ] = InstructionInfo( count++, 0x83F );
	g_InstructionMap[ "FPRINT" ] = InstructionInfo( count++, 0x415 );
	g_InstructionMap[ "FSIN"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FSQRT"  ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FSUB"   ] = InstructionInfo( count++, 0x83F );
	g_InstructionMap[ "FTAN"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "FTOI"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "GETCH"  ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "IAND"   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "IDIV"   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "IMOD"   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "IMUL"   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "INC"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "INT"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "IOR"	   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "IPRINT" ] = InstructionInfo( count++, 0x115 );
	g_InstructionMap[ "IXOR"   ] = InstructionInfo( count++, 0x23F );
	g_InstructionMap[ "JA"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JAE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JB"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JBE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JG"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JGE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JL"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JLE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JMP"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JNE"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JNO"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JNS"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JO"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JS"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "JZ"	   ] = InstructionInfo( count++, 0x40 );
	g_InstructionMap[ "LEA"	   ] = InstructionInfo( count++, 0x3F );
	g_InstructionMap[ "MOD"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "MOV"	   ] = InstructionInfo( count++, 0xABF );
	g_InstructionMap[ "MUL"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "NEG"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "NOP"	   ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "NOT"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "OR"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "POP"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "PROLOG" ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "PUSH"   ] = InstructionInfo( count++, 0x555 );
	g_InstructionMap[ "RCLR"   ] = InstructionInfo( count++, 64 );
	g_InstructionMap[ "RET"	   ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "RGET"   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "RPLOT"  ] = InstructionInfo( count++, 0x55 );
	g_InstructionMap[ "RPOS"   ] = InstructionInfo( count++, 0x33F );
	g_InstructionMap[ "SAL"	   ] = InstructionInfo( count++, 0x33F );
	g_InstructionMap[ "SAR"	   ] = InstructionInfo( count++, 0x33F );
	g_InstructionMap[ "SHL"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "SHR"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "SIF"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "SLEEP"  ] = InstructionInfo( count++, 0x55 );
	g_InstructionMap[ "SUB"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "TEST"   ] = InstructionInfo( count++, 0x3FF );
	g_InstructionMap[ "TIME"   ] = InstructionInfo( count++, 0 );
	g_InstructionMap[ "UIF"	   ] = InstructionInfo( count++, 0x15 );
	g_InstructionMap[ "UPRINT" ] = InstructionInfo( count++, 0x55 );
	g_InstructionMap[ "XADD"   ] = InstructionInfo( count++, 0x3F );
	g_InstructionMap[ "XCHG"   ] = InstructionInfo( count++, 0x3F );
	g_InstructionMap[ "XOR"	   ] = InstructionInfo( count++, 0xBF );
	g_InstructionMap[ "LGA"	   ] = InstructionInfo( count++, 0x35 );

	return 0;
};
extern void AddData( std::string flexString, InputTypes::InputType type );
extern unsigned int g_CurrentState;
extern ProgramLine g_CurrentLine;
extern std::map< std::string, GlobalVar* > g_GlobalVarMap;