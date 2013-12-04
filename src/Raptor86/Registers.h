// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

#include <memory>

namespace Raptor
{
	namespace r86
	{
		// Flags used for comparison functions.
		union ProcessorFlags
		{
			unsigned int pf_Flags[4];
			bool pf_FlagBits[32];
		};
		
		// This struct contains all the register related data.
		struct Registers
		{
			Registers( void )
				:
			r_InstructionPointer( 0 )
			{
				ClearAllRegisters();
				ClearAllFlags();

				r_FlagCF = (unsigned char*) &r_ProcessorFlags.pf_FlagBits[0];
				r_FlagZF = (unsigned char*) &r_ProcessorFlags.pf_FlagBits[1];
				r_FlagSF = (unsigned char*) &r_ProcessorFlags.pf_FlagBits[2];
				r_FlagOF = (unsigned char*) &r_ProcessorFlags.pf_FlagBits[3];

				r_BasePointer = &r_GeneralRegisters[128];
				r_StackPointer = &r_GeneralRegisters[129];
			}

			// General registers and BP, SP (131 * 4 bytes)
			unsigned int r_GeneralRegisters[131];
			unsigned int* r_BasePointer;
			unsigned int* r_StackPointer;

			ProcessorFlags r_ProcessorFlags;

			// Instruction pointer, you cannot modify this at run-time.
			unsigned int r_InstructionPointer;

			// Flags for the comparison functions.
			unsigned char* r_FlagCF;
			unsigned char* r_FlagZF;
			unsigned char* r_FlagSF;
			unsigned char* r_FlagOF;

			// Screen position for plotting. Optional.
			int r_RPosX;
			int r_RPosY;

			// Function to reset the values of all registers to 0.
			void ClearAllRegisters( void )
			{
				memset( r_GeneralRegisters, 0, sizeof( unsigned int ) * 128 );
			
				r_RPosX = -1;
				r_RPosY = -1;
			}

			// Resets all the compare flags. Done with every CMP/TEST.
			void ClearAllFlags( void )
			{
				r_ProcessorFlags.pf_Flags[0] = 0;
			}
		};
	};
};

