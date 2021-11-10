#include "Compiler.h"
#include "LeaMath.h"
#include <algorithm>
#include <Windows.h>

// Disable some warnings
#pragma warning(disable : 4311)
#pragma warning(disable : 4302)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4996)
#pragma warning(disable : 4477)

extern int yylineno;

#define R86A_PRINTF( x, ... ) \
	printf( "(Line: %d) " x, yylineno, ##__VA_ARGS__ );

unsigned int g_CurrentState = States::STATE_NONE;

std::vector< LabelInfo* > g_UnboundLabel;
std::map< std::string, LabelInfo* > g_LabelMap;

ProgramLine g_CurrentLine;

std::vector< ProgramLine > g_ProgramLines;
std::map< std::string, GlobalVar* > g_GlobalVarMap;

unsigned int g_GlobalAddress = 0;

InstructionInfo g_CurrentInstruction;
std::map< std::string, InstructionInfo > g_InstructionMap;

unsigned int g_Dummy = FillInstructionMap();

extern void AddData( std::string flexString, InputTypes::InputType type );

bool g_Halt = false;

using namespace InputTypes;
using namespace ProgramLineFlags;

LeaInstructionChain g_LeaChain;

unsigned int GetFlags( InputTypes::InputType type )
{
	unsigned int flags = 0;

	switch ( type )
	{
	case INPUT_OPERAND_UINT:
		flags = 64;
		break;

	case INPUT_OPERAND_INT:
		flags = 256;
		break;

	case INPUT_OPERAND_FLOAT:
		flags = 1024;
		break;

	case INPUT_OPERAND_DF_BASE_PTR:
	case INPUT_OPERAND_DF_STACK_PTR:
	case INPUT_OPERAND_DF_REGISTER:
		flags = 1;
		break;

	case INPUT_OPERAND_DF_GVAR:
	case INPUT_OPERAND_DF_ADDRESS:
		flags = 4;
		break;

	case INPUT_OPERAND_BASE_PTR:
	case INPUT_OPERAND_STACK_PTR:
	case INPUT_OPERAND_REGISTER:
		flags = 0;
		break;

	case INPUT_OPERAND_GVAR:
	case INPUT_OPERAND_ADDRESS:
		flags = 16;
		break;
	};

	if ( type == INPUT_OPERAND_GVAR || type == INPUT_OPERAND_DF_GVAR )
	{
		flags |= 4096;
	}

	return flags;
}

void ResolveGvar( std::string flexString )
{
	std::string identifier;
	std::string value;

	GlobalVar* newVar = new GlobalVar();
	newVar->gv_Address = g_GlobalAddress;
	newVar->gv_Value = 0;

	size_t sizeOfVal = 4;
	bool isInteger = true;

	unsigned int n = 0;
	char* p = 0;

	float f = 0;
	int i = 0;

	unsigned int state = 0;

	for ( unsigned int i = 1; i < flexString.size(); i++ )
	{
		switch ( state )
		{
		case 0:
			if ( flexString[i] == ' ' || flexString[i] == '\t' )
			{
				state = 1;
				break;
			}

			identifier.push_back( flexString[i] );

			break;

		case 1:
			if ( flexString[i] == '=' )
			{
				state = 2;
			}

			break;

		case 2:
			if ( flexString[i] != ' ' && flexString[i] != '\t' )
			{
				i--;
				state = 3;
				break;
			}

			break;

		case 3:
			value.push_back( flexString[i] );
			break;
		};
	}

	switch ( value.back() )
	{
	default:
		n = strtoul( value.c_str(), &p, 10 );
		break;

	case 'f':
		f = atof( value.c_str() );
		n = *(unsigned int*) &f;
		break;

	case 's':
		i = atoi( value.c_str() );
		n = *(unsigned int*) &i;
		break;

	case '\"':
		{
			// Special directive
			char directive[256];
			char value2[256];

			sscanf(value.c_str(), "%[^\" ] \"%[^\"]\"", directive, value2);

			if (strcmp(directive, "incbin") == 0)
			{
				// Include binary
				HANDLE f = CreateFileA(value2, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);

				if (f != INVALID_HANDLE_VALUE)
				{
					LARGE_INTEGER fileSize;
					GetFileSizeEx(f, &fileSize);
					
					size_t sz = fileSize.QuadPart;
					
					// Allocate space
					newVar->gv_Value = new char[sz];
					memset(newVar->gv_Value, 0, sz);

					sizeOfVal = sz;

					DWORD bread = 0;
					ReadFile(f, newVar->gv_Value, sz, &bread, NULL);

					CloseHandle(f);
				}
			}

			isInteger = false;
		}
		break;
	}

	if (isInteger)
		newVar->gv_Value = (char*)(new unsigned int(n));

	g_GlobalAddress += sizeOfVal;
	newVar->gv_Size = sizeOfVal;

	if ( g_GlobalVarMap.find( identifier ) != g_GlobalVarMap.end() )
	{
		g_GlobalAddress -= 4;
		R86A_PRINTF( "Warning: Multiple definition of global variable (%s). Newest discarded.\n", identifier.c_str() );
		delete newVar;
		return;
	}

	g_GlobalVarMap[ identifier ] = newVar;
}

void StateNone( std::string flexString, InputTypes::InputType type )
{
	switch ( type )
	{
	case INPUT_GVAR:
		ResolveGvar( flexString );
		return;

	case INPUT_NEW_LINE:
		return;

	case INPUT_INSTRUCTION:
		std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

		while ( flexString[0] == ' ' || flexString[0] == '\t' )
		{
			flexString = flexString.c_str() + 1;
		}

		if ( g_InstructionMap.find( flexString ) != g_InstructionMap.end() )
		{
			g_CurrentLine.pl_Opcode = g_InstructionMap[ flexString ].ii_Opcode;
			g_CurrentState = States::STATE_INSTRUCTION_SPECIFIED;

			g_CurrentLine.pl_Flags = 0;
			g_CurrentLine.pl_Operand1 = 0;
			g_CurrentLine.pl_Operand2 = 0;
			g_CurrentLine.pl_Operand1Label = "";
			g_CurrentLine.pl_Operand2Label = "";
			g_CurrentLine.pl_Operand1Set = false;

			g_CurrentInstruction = g_InstructionMap[ flexString ];

			if ( g_UnboundLabel.size() != 0 )
			{
				for ( unsigned int i = 0; i < g_UnboundLabel.size(); i++ )
				{
					g_UnboundLabel[i]->li_Line = g_ProgramLines.size();
				}

				g_UnboundLabel.clear();
			}

			return;
		}

		g_Halt = true;
		R86A_PRINTF( "Error: Invalid instruction (%s)! Failed to compile.\n", flexString.c_str() );
	
		return;

	case INPUT_LABEL:
		LabelInfo* newLabel = new LabelInfo();
		newLabel->li_Identifier = flexString;

		while ( flexString[0] == ' ' || flexString[0] == '\t' )
		{
			flexString = flexString.c_str() + 1;
		}

		if ( g_LabelMap.find( flexString ) != g_LabelMap.end() )
		{
			R86A_PRINTF( "Error: Multiple definition of label (%s)! Failed to compile.\n", flexString.c_str() );
			g_Halt = true;

			return;
		}

		g_LabelMap[ flexString ] = newLabel;
		g_UnboundLabel.push_back( newLabel );

		return;
	}

	R86A_PRINTF( "Warning: Unrecognized Input (%s)!\n", flexString.c_str() );
}

void StateInstruction( std::string flexString, InputTypes::InputType type )
{
	if ( type == INPUT_INSTRUCTION )
	{
		g_ProgramLines.push_back( g_CurrentLine );
		g_CurrentState = States::STATE_NONE;
		StateNone( flexString, type );

		return;
	}

	if ( g_CurrentInstruction.ii_QualifFlags == 0 && type == INPUT_NEW_LINE )
	{
		g_CurrentLine.pl_Operand1Set = false;
		g_ProgramLines.push_back( g_CurrentLine );
		g_CurrentState = States::STATE_NONE;
		return;
	}

	else if ( type == INPUT_LABEL )
	{
		g_ProgramLines.push_back( g_CurrentLine );
		g_CurrentState = States::STATE_NONE;
		StateNone( flexString, type );

		return;
	}

	unsigned int flags = GetFlags( type );
	unsigned int qualifFlags = flags & g_CurrentInstruction.ii_QualifFlags;

	char* p;
	unsigned int n;
	int i;
	float f;

	if ( type == INPUT_OPERAND_LABEL && ( g_CurrentInstruction.ii_QualifFlags & 64 ) )
	{
		g_CurrentLine.pl_Operand1Set = true;
		g_CurrentLine.pl_Operand1Label = flexString;
		g_CurrentLine.pl_Flags |= 64;
	}

	else
	{
		g_CurrentLine.pl_Operand1Set = true;

		switch ( qualifFlags )
		{
		case 0:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

			if ( flexString.compare( "SP" ) == 0 )
			{
				g_CurrentLine.pl_Operand1 = 129;
			}

			else if ( flexString.compare( "BP" ) == 0 )
			{
				g_CurrentLine.pl_Operand1 = 128;
			}

			else if ( flexString[0] == 'R' )
			{
				unsigned int regNum = atoi( flexString.c_str() + 1 );

				if ( regNum > 127 )
				{
					R86A_PRINTF( "Error: Register may not exceed 127! (%d)\n", regNum );
					g_Halt = true;
					return;
				}

				g_CurrentLine.pl_Operand1 = regNum;
			}

			break;

		case 1:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

			if ( flexString.compare( "[SP]" ) == 0 )
			{
				g_CurrentLine.pl_Flags |= 1;
				g_CurrentLine.pl_Operand1 = 129;
			}

			else if ( flexString.compare( "[BP]" ) == 0 )
			{
				g_CurrentLine.pl_Flags |= 1;
				g_CurrentLine.pl_Operand1 = 129;
			}

			else if ( flexString[1] == 'R' )
			{
				g_CurrentLine.pl_Flags |= 1;

				unsigned int regNum = atoi( flexString.c_str() + 2 );

				if ( regNum > 127 )
				{
					R86A_PRINTF( "Error: Register may not exceed 127! (%d)\n", regNum );
					g_Halt = true;
					return;
				}

				g_CurrentLine.pl_Operand1 = regNum;
			}

			break;

		case 4:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), tolower );

			g_CurrentLine.pl_Flags |= 4;

			if ( flexString.compare( 0, 4,"[%0x" ) == 0 )
			{
				std::string newString = flexString.c_str() + 4;
				newString.pop_back();

				p;
				n = strtoul( newString.c_str(), &p, 16 );

				g_CurrentLine.pl_Operand1 = n;
			}

			else
			{
				std::string newString = flexString.c_str() + 2;
				newString.pop_back();

				p;
				n = strtoul( newString.c_str(), &p, 10 );

				g_CurrentLine.pl_Operand1 = n;
			}

			break;

		case 16:
			g_CurrentLine.pl_Flags |= 16;

			if ( type == INPUT_OPERAND_GVAR )
			{
				g_CurrentLine.pl_Flags |= 4096;

				flexString = flexString.c_str() + 1;

				if ( g_GlobalVarMap.find( flexString ) != g_GlobalVarMap.end() )
				{
					g_CurrentLine.pl_Operand1 = g_GlobalVarMap[ flexString ]->gv_Address;
				}

				else
				{
					g_Halt = true;
					R86A_PRINTF( "Error: Global variable (%s) does not exist! Failed to compile!\n", flexString.c_str() );
					return;
				}
			}

			else
			{
				std::transform( flexString.begin(), flexString.end(), flexString.begin(), tolower );

				if ( flexString.compare( 0, 3,"%0x" ) == 0 )
				{
					std::string newString = flexString.c_str() + 3;

					char* p;
					long n = strtoul( newString.c_str(), &p, 16 );

					g_CurrentLine.pl_Operand1 = n;
				}

				else
				{
					std::string newString = flexString.c_str() + 1;

					char* p;
					long n = strtoul( newString.c_str(), &p, 10 );

					g_CurrentLine.pl_Operand1 = n;
				}
			}

			break;

		case 64:
			g_CurrentLine.pl_Flags |= 64;

			if ( flexString.compare( 0, 2, "0x" ) == 0 )
			{
				std::string newString = flexString.c_str() + 2;

				p;
				n = strtoul( newString.c_str(), &p, 16 );
			}

			else
			{
				p;
				n = strtoul( flexString.c_str(), &p, 10 );
			}

			g_CurrentLine.pl_Operand1 = n;

			break;

		case 256:
			g_CurrentLine.pl_Flags |= 256;

			i = atoi( flexString.c_str() );

			g_CurrentLine.pl_Operand1 = *(unsigned int*) &i;

			break;

		case 1024:
			g_CurrentLine.pl_Flags |= 1024;

			f = atof( flexString.c_str() );

			g_CurrentLine.pl_Operand1 = *(unsigned int*) &f;

			break;
		}
	}

	g_CurrentState = States::STATE_OPERAND1_SPECIFIED;

	if ( ( g_CurrentInstruction.ii_QualifFlags & ( 2|8|32|128|512|2048|8192 ) ) == 0 )
	{
		g_CurrentState = States::STATE_NONE;
		g_ProgramLines.push_back( g_CurrentLine );
	}

	return;
}

void StateOperand1( std::string flexString, InputTypes::InputType type )
{
	if ( type == INPUT_INSTRUCTION )
	{
		g_ProgramLines.push_back( g_CurrentLine );
		g_CurrentState = States::STATE_NONE;
		StateNone( flexString, type );

		return;
	}

	unsigned int flags = GetFlags( type );
	flags <<= 1;

	unsigned int qualifFlags = flags & g_CurrentInstruction.ii_QualifFlags;

	char* p;
	unsigned int n;
	int i;
	float f;

	if ( type == INPUT_OPERAND_LABEL && ( g_CurrentInstruction.ii_QualifFlags & 128 ) )
	{
		g_CurrentLine.pl_Operand2Label = flexString;
		g_CurrentLine.pl_Flags |= 128;
	}

	else
	{
		switch ( qualifFlags )
		{
		case 0:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

			if ( flexString.compare( "SP" ) == 0 )
			{
				g_CurrentLine.pl_Operand2 = 129;
			}

			else if ( flexString.compare( "BP" ) == 0 )
			{
				g_CurrentLine.pl_Operand2 = 128;
			}

			else if ( flexString[0] == 'R' )
			{
				unsigned int regNum = atoi( flexString.c_str() + 1 );

				if ( regNum > 127 )
				{
					R86A_PRINTF( "Error: Register may not exceed 127! (%d)\n", regNum );
					g_Halt = true;
					return;
				}

				g_CurrentLine.pl_Operand2 = regNum;
			}

			break;

		case 2:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

			if ( flexString.compare( "[SP]" ) == 0 )
			{
				g_CurrentLine.pl_Flags |= 2;
				g_CurrentLine.pl_Operand2 = 129;
			}

			else if ( flexString.compare( "[BP]" ) == 0 )
			{
				g_CurrentLine.pl_Flags |= 2;
				g_CurrentLine.pl_Operand2 = 129;
			}

			else if ( flexString[1] == 'R' )
			{
				g_CurrentLine.pl_Flags |= 2;

				unsigned int regNum = atoi( flexString.c_str() + 2 );

				if ( regNum > 127 )
				{
					R86A_PRINTF( "Error: Register may not exceed 127! (%d)\n", regNum );
					g_Halt = true;
					return;
				}

				g_CurrentLine.pl_Operand2 = regNum;
			}

			break;

		case 8:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), tolower );

			g_CurrentLine.pl_Flags |= 8;

			if ( flexString.compare( 0, 4,"[%0x" ) == 0 )
			{
				std::string newString = flexString.c_str() + 4;
				newString.pop_back();

				char* p;
				long n = strtoul( newString.c_str(), &p, 16 );

				g_CurrentLine.pl_Operand2 = n;
			}

			else
			{
				std::string newString = flexString.c_str() + 2;
				newString.pop_back();

				char* p;
				long n = strtoul( newString.c_str(), &p, 10 );

				g_CurrentLine.pl_Operand2 = n;
			}

			break;

		case 32:
			g_CurrentLine.pl_Flags |= 32;

			if ( type == INPUT_OPERAND_GVAR )
			{
				g_CurrentLine.pl_Flags |= 8192;

				flexString = flexString.c_str() + 1;

				if ( g_GlobalVarMap.find( flexString ) != g_GlobalVarMap.end() )
				{
					g_CurrentLine.pl_Operand2 = g_GlobalVarMap[ flexString ]->gv_Address;
				}

				else
				{
					g_Halt = true;
					R86A_PRINTF( "Error: Global variable (%s) does not exist! Failed to compile!\n", flexString.c_str() );
					return;
				}
			}

			else
			{
				std::transform( flexString.begin(), flexString.end(), flexString.begin(), tolower );

				if ( flexString.compare( 0, 3,"%0x" ) == 0 )
				{
					std::string newString = flexString.c_str() + 3;

					char* p;
					long n = strtoul( newString.c_str(), &p, 16 );

					g_CurrentLine.pl_Operand2 = n;
				}

				else
				{
					std::string newString = flexString.c_str() + 1;

					char* p;
					long n = strtoul( newString.c_str(), &p, 10 );

					g_CurrentLine.pl_Operand2 = n;
				}
			}

			break;

		case 128:
			g_CurrentLine.pl_Flags |= 128;

			if ( flexString.compare( 0, 2, "0x" ) == 0 )
			{
				std::string newString = flexString.c_str() + 2;

				p;
				n = strtoul( newString.c_str(), &p, 16 );
			}

			else
			{
				p;
				n = strtoul( flexString.c_str(), &p, 10 );
			}

			g_CurrentLine.pl_Operand2 = n;

			break;

		case 512:
			g_CurrentLine.pl_Flags |= 512;

			i = atoi( flexString.c_str() );

			g_CurrentLine.pl_Operand2 = *(unsigned int*) &i;

			break;

		case 2048:
			g_CurrentLine.pl_Flags |= 2048;

			f = atof( flexString.c_str() );

			g_CurrentLine.pl_Operand2 = *(unsigned int*) &f;

			break;
		}
	}

	g_CurrentState = States::STATE_NONE;
	g_ProgramLines.push_back( g_CurrentLine );

	return;
}

void StateLeaData( std::string flexString, InputTypes::InputType type )
{
	if ( type == INPUT_LEA_INTERNAL )
	{
		g_CurrentState = States::STATE_LEA_INTERNAL;
		return;
	}

	g_CurrentInstruction = g_InstructionMap[ "LEA" ];

	StateInstruction( flexString, type );
	g_CurrentState = States::STATE_LEA_DATA;
}

void StateLeaInternal( std::string flexString, InputTypes::InputType type )
{
	if ( flexString[0] == ']' )
	{
		g_CurrentState = States::STATE_NONE;

		LeaInstructions li = ExpandLeaChain( g_CurrentLine, g_LeaChain );

		for ( unsigned int i = 0; i < li.li_SubLines.size(); i++ )
		{
			g_ProgramLines.push_back( li.li_SubLines[i] );
		}

		g_CurrentLine = ProgramLine();

		return;
	}

	unsigned int flags = GetFlags( type );
	unsigned int qualifFlags = flags & ( OPERAND1_HEAP_ADDRESS | OPERAND1_RAW_UNSIGNED_NUMBER );

	if ( type == INPUT_LEA_MATH )
	{
		LeaToken token;

		token.lt_TokenString = flexString;
		token.lt_InputType = type;

		g_LeaChain.lic_Tokens.push_back( token );

		return;
	}

	else
	{
		unsigned int rawOperand = 0;

		LeaToken token;

		token.lt_TokenString = flexString;
		token.lt_InputType = type;
		
		unsigned int newFlags = 0;

		switch ( qualifFlags )
		{
		default:
			R86A_PRINTF( "Error: Operand not supported in LEA!\n" );
			g_Halt = true;
			return;

		case 0:
			std::transform( flexString.begin(), flexString.end(), flexString.begin(), toupper );

			if ( flexString[0] == 'R' )
			{
				unsigned int regNum = atoi( flexString.c_str() + 1 );

				if ( regNum > 127 )
				{
					R86A_PRINTF( "Error: Register may not exceed 127! (%d)\n", regNum );
					g_Halt = true;
					return;
				}

				rawOperand = regNum;
			}

			break;

		case 16:
			newFlags |= 16;

			if ( type == INPUT_OPERAND_GVAR )
			{
				newFlags |= 4096;

				flexString = flexString.c_str() + 1;

				if ( g_GlobalVarMap.find( flexString ) != g_GlobalVarMap.end() )
				{
					rawOperand = g_GlobalVarMap[ flexString ]->gv_Address;
				}

				else
				{
					g_Halt = true;
					R86A_PRINTF( "Error: Global variable (%s) does not exist! Failed to compile!\n", flexString.c_str() );
					return;
				}
			}

			else
			{
				std::transform( flexString.begin(), flexString.end(), flexString.begin(), tolower );

				if ( flexString.compare( 0, 3,"%0x" ) == 0 )
				{
					std::string newString = flexString.c_str() + 3;

					char* p;
					long n = strtoul( newString.c_str(), &p, 16 );

					rawOperand = n;
				}

				else
				{
					std::string newString = flexString.c_str() + 1;

					char* p;
					long n = strtoul( newString.c_str(), &p, 10 );

					rawOperand = n;
				}
			}

			break;

		case 64:
			newFlags |= 64;

			long n = 0;

			if ( flexString.compare( 0, 2, "0x" ) == 0 )
			{
				std::string newString = flexString.c_str() + 2;

				char* p;
				n = strtoul( newString.c_str(), &p, 16 );
			}

			else
			{
				char* p;
				n = strtoul( flexString.c_str(), &p, 10 );
			}

			rawOperand = n;

			break;
		}

		token.lt_Flags = (ProgramLineFlag) qualifFlags;
		token.lt_Raw = rawOperand;

		token.lt_Flags = (ProgramLineFlag) newFlags;

		g_LeaChain.lic_Tokens.push_back( token );
	}
}

void AddData( std::string flexString, InputTypes::InputType type )
{
	if ( g_Halt ) return;

	if ( type == INPUT_LEA_INSTRUCTION )
	{
		g_CurrentLine = ProgramLine();
		g_CurrentState = States::STATE_LEA_DATA;
		g_LeaChain = LeaInstructionChain();
		return;
	}

	if ( type == INPUT_INSTRUCTION && g_CurrentState == States::STATE_INSTRUCTION_SPECIFIED )
	{
		g_CurrentLine.pl_Operand1Set = true;
		g_ProgramLines.push_back( g_CurrentLine );
		g_CurrentState = States::STATE_NONE;
	}

	if ( type == INPUT_NEW_LINE )
	{
		g_CurrentState = States::STATE_NONE;
		return;
	}

	switch ( g_CurrentState )
	{
	case States::STATE_NONE:
		StateNone( flexString, type );
		break;

	case States::STATE_LEA_DATA:
		StateLeaData( flexString, type );
		break;

	case States::STATE_LEA_INTERNAL:
		StateLeaInternal( flexString, type );
		break;

	case States::STATE_INSTRUCTION_SPECIFIED:
		StateInstruction( flexString, type );
		break;

	case States::STATE_OPERAND1_SPECIFIED:
		StateOperand1( flexString, type );
		break;
	};
}