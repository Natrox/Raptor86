#include "LeaMath.h"
#include "Instructions.h"

using namespace Instructions;
using namespace ProgramLineFlags;

#ifdef _DEBUG
#define ASM_S( x ) case x : \
		inst = #x ; \
		break
#endif

enum MathOperation
{
	MATH_NONE,
	MATH_ADD,
	MATH_SUB,
	MATH_MUL
};

Instructions::Instruction g_MathToInst[4] =
{
	ASM_ADD,
	ASM_ADD,
	ASM_SUB,
	ASM_MUL
};

struct LeaTokenGroup
{
	LeaTokenGroup( void )
	{
		ltp_MathOperationPre = MATH_ADD;
		ltp_MathOperationGroup = MATH_NONE;
	}

	MathOperation ltp_MathOperationPre;
	MathOperation ltp_MathOperationGroup;
	std::vector< LeaToken > ltp_Group;
};

MathOperation GetMathOperation( std::string x )
{
	switch ( x[0] )
	{
	case '+':
		return MATH_ADD;

	case '-':
		return MATH_SUB;

	case '*':
		return MATH_MUL;
	};

	return MATH_NONE;
}

std::string SumLeaConstants( std::string a, std::string b, std::string operation )
{
	size_t aa = 0;
	size_t bb = 0;

	stoi( a, &aa );
	stoi( b, &bb );

	char str[32];

	switch ( operation[0] )
	{
	case '+':
		sprintf( str, "%u", unsigned int(aa + bb) );
		break;

	case '-':
		sprintf( str, "%u", unsigned int(aa - bb) );
		break;

	case '*':
		sprintf( str, "%u", unsigned int(aa * bb) );
		break;

	default:
		break;
	}

	return str;
}

LeaInstructions ExpandLeaChain( ProgramLine leaInstruction, LeaInstructionChain chain )
{
	//// Concatenate

	//while ( true )
	//{
	//	bool found = false;

	//	for ( auto i = chain.lic_Tokens.begin(); i != chain.lic_Tokens.end(); i++ )
	//	{
	//		if ( i->lt_TokenString.compare( "+" ) == 0 || i->lt_TokenString.compare( "-" ) == 0 || i->lt_TokenString.compare( "*" ) == 0 )
	//		{
	//			if ( std::next( i, -1 )->lt_InputType == InputTypes::INPUT_OPERAND_UINT && std::next( i, 1 )->lt_InputType == InputTypes::INPUT_OPERAND_UINT )
	//			{
	//				std::string result = SumLeaConstants( std::next( i, -1 )->lt_TokenString, std::next( i, 1 )->lt_TokenString, i->lt_TokenString );
	//				found = true;

	//				chain.lic_Tokens.erase( std::next( i, -1 ) );
	//				chain.lic_Tokens.erase( std::next( i, 1 ) );

	//				LeaToken newToken;

	//				newToken.lt_InputType = InputTypes::INPUT_OPERAND_UINT;
	//				newToken.lt_TokenString = result;

	//				*i = newToken;

	//				break;
	//			}
	//		}
	//	}

	//	if ( found == false ) break;
	//}

	LeaTokenGroup currentGroup;
	std::vector< LeaTokenGroup > groups;
	bool addGroup = false;

	for ( auto i = chain.lic_Tokens.begin(); i != chain.lic_Tokens.end(); i++ )
	{
		if ( i->lt_TokenString.compare( "+" ) == 0 || i->lt_TokenString.compare( "-" ) == 0 )
		{
			groups.push_back( currentGroup );
			currentGroup = LeaTokenGroup();
			currentGroup.ltp_MathOperationPre = GetMathOperation( i->lt_TokenString );
		}

		else if ( i->lt_TokenString.compare( "*" ) == 0 )
		{
			currentGroup.ltp_MathOperationGroup = MATH_MUL;
		}

		else
		{
			currentGroup.ltp_Group.push_back( *i );
		}
	}

	groups.push_back( currentGroup );

	auto createInstructionExtended = [&] ( unsigned short opcode, unsigned int flags, unsigned int operand1, unsigned int operand2 )
	{
		ProgramLine newLine;

		newLine.pl_Opcode = opcode;
		newLine.pl_Operand1 = operand1;
		newLine.pl_Operand2 = operand2;
		newLine.pl_Flags = flags;

#ifdef _DEBUG
		std::string inst;

		switch ( opcode )
		{
			ASM_S( ASM_MOV );
			ASM_S( ASM_MUL );
			ASM_S( ASM_ADD );
			ASM_S( ASM_SUB );
			ASM_S( ASM_PUSH );
			ASM_S( ASM_XOR );
			ASM_S( ASM_POP );
		};

		printf( "%s / %d / %d / %d\n", inst.c_str(), flags, operand1, operand2 );
#endif

		return newLine;
	};

	LeaInstructions instructions;

	instructions.li_SubLines.push_back(createInstructionExtended(ASM_PUSH, 0, 0, 0));
	instructions.li_SubLines.push_back(createInstructionExtended(ASM_PUSH, 0, 2, 0));

	instructions.li_SubLines.push_back( createInstructionExtended( ASM_XOR, leaInstruction.pl_Flags | ( leaInstruction.pl_Flags << 1 ), 
																   leaInstruction.pl_Operand1, leaInstruction.pl_Operand1 ) );

	for ( unsigned int i = 0; i < groups.size(); i++ )
	{
		for ( unsigned int b = 0; b < groups[i].ltp_Group.size(); b++ )
		{
			if ( groups[i].ltp_Group.size() == 1 )
			{
				Instructions::Instruction inst = g_MathToInst[ groups[i].ltp_MathOperationPre ];
				instructions.li_SubLines.push_back( createInstructionExtended( inst, leaInstruction.pl_Flags | ( groups[i].ltp_Group[b].lt_Flags << 1 ), 
																			   leaInstruction.pl_Operand1, groups[i].ltp_Group[b].lt_Raw ) );
			}

			else
			{
				if ( b == 0 )
				{
					instructions.li_SubLines.push_back( createInstructionExtended( ASM_MOV, OPERAND1_DEREFERENCE_REGISTER | ( groups[i].ltp_Group[b].lt_Flags << 1 ), 
																				   129, groups[i].ltp_Group[b].lt_Raw ) );
				}

				else
				{
					instructions.li_SubLines.push_back( createInstructionExtended( ASM_MUL, OPERAND1_DEREFERENCE_REGISTER | ( groups[i].ltp_Group[b].lt_Flags << 1 ), 
																				   129, groups[i].ltp_Group[b].lt_Raw ) );
				}

				if ( b == groups[i].ltp_Group.size() - 1 )
				{
					Instructions::Instruction inst = g_MathToInst[ groups[i].ltp_MathOperationPre ];
					instructions.li_SubLines.push_back( createInstructionExtended( inst, leaInstruction.pl_Flags | OPERAND2_DEREFERENCE_REGISTER, 
																				   leaInstruction.pl_Operand1, 129 ) );
				}
			}
		}
	}

	instructions.li_SubLines.push_back( createInstructionExtended( ASM_POP, 0, 2, 0 ) );
	instructions.li_SubLines.push_back( createInstructionExtended( ASM_POP, 0, 0, 0 ) );

	return instructions;
}
