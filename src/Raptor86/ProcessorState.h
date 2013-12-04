// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

namespace Raptor
{
	namespace r86
	{
		// This struct contains the neccessary information to execute a program line.
		struct ProcessorState
		{
			ProcessorState( void )
				:
			ps_Halt( false ),
			ps_Operand1Ptr( 0 ),
			ps_Operand2Ptr( 0 )
			{}

			// Halt boolean. Set whenever an access violation occurs.
			bool ps_Halt;

			// Information about the current program line.
			unsigned int ps_ProgramLineOpcode;
			unsigned int ps_ProgramLineFlags;
			unsigned int ps_Operand1;
			unsigned int ps_Operand2;

			// Temporary variables for arithmetic and logical computations.
			unsigned int ps_UIOperand1;
			unsigned int ps_UIOperand2;
			
			int ps_IOperand1;
			int ps_IOperand2;

			float ps_FOperand1;
			float ps_FOperand2;

			// Pointers which resolve to a register or heap location.
			void* ps_Operand1Ptr;
			void* ps_Operand2Ptr;
		};
	};
};