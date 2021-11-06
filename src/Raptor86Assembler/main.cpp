#include "Tokenizer.h"
#include <Windows.h>

#define argu argv[1]
//#define argu "maze.rasm"

int main( int argc, char *argv[] )
{
	if ( argc <= 1 )
	{
		printf( "Usage: r86.exe <filename>. Or drag file into executable." );
		getchar();

		return 1;
	}

	printf( "Assembling: %s.\n", argu );

	ParseR86File( argu );

	std::string newString = argu;

	while ( newString.back() != '.' )
	{
		newString.pop_back();
	}

	newString.append( "r86" );

	if ( g_CurrentLine.pl_Operand1Set == false )
	{
		g_ProgramLines.push_back( g_CurrentLine );
	}

	if ( g_UnboundLabel.size() != 0 )
	{
		ProgramLine pl;

		memset( &pl, 0, sizeof( ProgramLine ) );

		pl.pl_Opcode = Instructions::ASM_NOP;
		g_ProgramLines.push_back( pl );

		for ( unsigned int i = 0; i < g_UnboundLabel.size(); i++ )
			g_UnboundLabel[i]->li_Line = unsigned int(g_ProgramLines.size() - 1);
	}

	for ( unsigned int i = 0; i < g_ProgramLines.size(); i++ )
	{
		if ( g_ProgramLines[i].pl_Operand1Label.compare( "" ) != 0 )
		{
			if ( g_LabelMap.find( g_ProgramLines[i].pl_Operand1Label ) != g_LabelMap.end() )
			{
				g_ProgramLines[i].pl_Operand1 = g_LabelMap[ g_ProgramLines[i].pl_Operand1Label ]->li_Line; 
			}

			else
			{
				printf( "Error: Label (%s) does not exist!\n", g_ProgramLines[i].pl_Operand1Label.c_str() );
			}
		}

		if ( g_ProgramLines[i].pl_Operand2Label.compare( "" ) != 0 )
		{
			if ( g_LabelMap.find( g_ProgramLines[i].pl_Operand2Label ) != g_LabelMap.end() )
			{
				g_ProgramLines[i].pl_Operand2 = g_LabelMap[ g_ProgramLines[i].pl_Operand2Label ]->li_Line; 
			}

			else
			{
				printf( "Error: Label (%s) does not exist!\n", g_ProgramLines[i].pl_Operand2Label.c_str() );
			}
		}
	}

	unsigned int heapSize = 0;

	for (std::map< std::string, GlobalVar* >::iterator it = g_GlobalVarMap.begin(); it != g_GlobalVarMap.end(); it++)
		heapSize += unsigned int(it->second->gv_Size);

	unsigned char* heapSection = 0;

	if ( heapSize > 0 )
	{
		heapSection = (unsigned char*) malloc( heapSize );

		for ( std::map< std::string, GlobalVar* >::iterator it = g_GlobalVarMap.begin(); it != g_GlobalVarMap.end(); it++ )
		{
			void* ptr = &heapSection[ it->second->gv_Address ];
			memcpy(ptr, it->second->gv_Value, it->second->gv_Size);
			delete it->second;
		}
	}

	FILE* out = fopen( newString.c_str(), "wb" );

	unsigned int magicIdent = 0x00decafe;

	fwrite( &magicIdent, sizeof( unsigned int ), 1, out );
	fwrite( &heapSize, sizeof( unsigned int ), 1, out );

	if ( heapSize > 0 ) 
		fwrite( heapSection, heapSize, 1, out );

	unsigned int numInstructions = unsigned int(g_ProgramLines.size());

	fwrite( &numInstructions, sizeof( unsigned int ), 1, out );

	for ( unsigned int i = 0; i < g_ProgramLines.size(); i++ )
	{
		fwrite( &g_ProgramLines[i].pl_Opcode, sizeof( uint8_t ), 1, out );
		fwrite( &g_ProgramLines[i].pl_Flags, sizeof( uint16_t ), 1, out );
		fwrite( &g_ProgramLines[i].pl_Operand1, sizeof( unsigned int ), 1, out );
		fwrite( &g_ProgramLines[i].pl_Operand2, sizeof( unsigned int ), 1, out );
	}

	fclose( out );

	free( heapSection );
	
	printf( "Done assembling.\n" );
	getchar();
}