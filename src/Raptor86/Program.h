// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

#include "HeapInfo.h"

namespace Raptor
{
	namespace r86
	{
		// Struct packing is enabled because of future plans to output the programs.
		// Feel free to comment these out if you're worried about cache performance.	
		#pragma pack(push, 1)

		struct ProgramLine
		{
			ProgramLine( unsigned int dbgLineNo, unsigned short opcode, unsigned int flags, unsigned int operand1, unsigned int operand2 )
				:
			pl_DebugLineNumber( dbgLineNo ),
			pl_Opcode( opcode ),
			pl_Flags( flags ),
			pl_Operand1( operand1 ),
			pl_Operand2( operand2 )
			{}

			// Optional and not in use at the moment.
			unsigned int pl_DebugLineNumber;

			// All the data required for a program line to execute.
			unsigned short pl_Opcode;
			unsigned int pl_Flags;
			unsigned int pl_Operand1;
			unsigned int pl_Operand2;
		};

		#pragma pack(pop)

		namespace ProgramLineFlags
		{
			// Different pre-specified flags which explain what the operands mean.
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

			// Flag combinations used to determine if checking the heap or registers is needed.
			// Registers are not checked at run-time at this moment, for performance reasons.
			static const unsigned int plf_RawRegisterFlagOp1 = 0x555;
			static const unsigned int plf_RawRegisterFlagOp2 = 0xAAA;
					  	
			static const unsigned int plf_HeapCheckFlagsOp1 = 0x14;
			static const unsigned int plf_HeapCheckFlagsOp2 = 0x28;
		};

		struct Program
		{
			// Constructor which loads a program from a file.
			Program( const char* fileNameR86, HeapInfo* heap )
			{
				p_Halt = false;

				FILE* file;
				fopen_s( &file, fileNameR86, "rb" );

				// Read the heap section size and also read the heap section itself.
				// The heap section is a piece of memory reserved for global variables (for the assembler).
				// See DOCUMENTATION.

				unsigned int magicIdent = 0;

				fread( &magicIdent, sizeof( unsigned int ), 1, file );

				if ( magicIdent != 0x00decafe )
				{
					fclose( file );
					p_DeleteProgram = false;

					R86_PRINT( "R86-ERROR: Not a valid program! (%s).\n", fileNameR86 );
					p_Halt = true;
					
					return;
				}

				unsigned int heapSize = 0;

				fread( &heapSize, sizeof( unsigned int ), 1, file );

				if ( heapSize > 0 )
				{
					unsigned char* heapSection = (unsigned char*) malloc( heapSize );

					unsigned char* address = (unsigned char*) heap->hi_HeapPtr + heap->hi_HeapLengthInBytes - 1048576 - heapSize - 4;
					fread( heapSection, heapSize, 1, file );

					heap->hi_StaticHeapSectionRadix = heap->hi_HeapLengthInBytes - 1048576 - 4;
					heap->hi_StaticHeapSectionRadix -= heapSize;

					memcpy( address, heapSection, heapSize );

					free( heapSection );
				}

				fread( &p_NumberOfInstructions, sizeof( unsigned int ), 1, file );

				p_ProgramLines = (ProgramLine*) malloc( sizeof( ProgramLine ) * p_NumberOfInstructions );

				for ( unsigned int i = 0; i < p_NumberOfInstructions; i++ )
				{
					fread( &p_ProgramLines[i].pl_Opcode, sizeof( unsigned short ), 1, file );
					fread( &p_ProgramLines[i].pl_Flags, sizeof( unsigned int ), 1, file );
					fread( &p_ProgramLines[i].pl_Operand1, sizeof( unsigned int ), 1, file );
					fread( &p_ProgramLines[i].pl_Operand2, sizeof( unsigned int ), 1, file );
				}

				fclose( file );

				p_DeleteProgram = true;
			}

			Program( ProgramLine* programLines, unsigned int programSize, bool deleteProgramAfterEject = false )
				:
			p_ProgramLines( programLines ),
			p_NumberOfInstructions( programSize ),
			p_DeleteProgram( deleteProgramAfterEject )
			{}

			~Program( void )
			{
				if ( p_DeleteProgram )
				{
					free( p_ProgramLines );	
				}
			}

			ProgramLine* p_ProgramLines;
			unsigned int p_NumberOfInstructions;
			bool p_Halt;

		private:
			bool p_DeleteProgram;
		};
	
	};
};