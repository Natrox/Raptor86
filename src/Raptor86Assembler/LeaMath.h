#pragma once

#include "Instructions.h"
#include "Compiler.h"

#include <list>

struct LeaToken
{
	std::string lt_TokenString;
	unsigned int lt_Raw;
	ProgramLineFlags::ProgramLineFlag lt_Flags;
	InputTypes::InputType lt_InputType;
};

struct LeaInstructionChain
{
	std::list< LeaToken > lic_Tokens;
};

struct LeaInstructions
{
	std::vector< ProgramLine > li_SubLines;
};

LeaInstructions ExpandLeaChain( ProgramLine leaInstruction, LeaInstructionChain chain );