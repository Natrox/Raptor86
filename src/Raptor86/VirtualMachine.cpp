// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#include "VirtualMachine.h"

#include <time.h>

// Disable some warnings
#pragma warning(disable : 4311)
#pragma warning(disable : 4302)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4996)
#pragma warning(disable : 4477)

using namespace Raptor::r86;

// Wrapper, with exit condition
#define _CheckProgramLineFlags(flags, allowedFlags, operand1, operand2, operand1Val, operand2Val) \
	CheckProgramLineFlags<allowedFlags>(flags, operand1, operand2, operand1Val, operand2Val); \
	if (m_ProcessorState->ps_Halt) break;

namespace Raptor
{
	namespace r86
	{
		DWORD WINAPI StartVMThread( void* clientPtr )
		{
			VirtualMachine* vm = (VirtualMachine*) clientPtr;

			vm->m_ProcessorState->ps_Halt = false;
			vm->DispatchLoop();
			
			// Reset all VM state data.
			vm->m_Registers->ClearAllRegisters();
			vm->m_Registers->ClearAllFlags();

			vm->m_Registers->r_InstructionPointer = 0;

			return 1;
		}
	};
};

VirtualMachine::VirtualMachine( HeapMemorySizes::HeapMemorySize heapSize )
{
	m_PlotPixelFunction = 0;
	m_ScreenClearFunction = 0;
	m_GetPixelFunction = 0;
	m_CheckKeyFunction = 0;

	m_StopRequested = 0;

	memset( m_InterruptTable, 0, sizeof( InterruptFunction ) * 256 );

	m_ProcessorState = new ProcessorState();
	m_Registers = new Registers();
	m_HeapInfo = new HeapInfo( (unsigned int) heapSize + 1048576 );
	m_Stack = new Stack( m_HeapInfo, m_Registers, m_ProcessorState );

	m_CurrentProgram = 0;
	m_DeleteProgram = false;

	m_VMThread = 0;
	m_VMQuitEvent = CreateEvent( 0, false, false, 0 );
}

VirtualMachine::~VirtualMachine( void )
{
	StopVM();

	if ( m_DeleteProgram ) delete m_CurrentProgram;
	m_CurrentProgram = 0;

	delete m_ProcessorState;
	delete m_Registers;
	delete m_HeapInfo;
	delete m_Stack;
}

void VirtualMachine::StartVM( void )
{
	if ( m_VMThread == 0 )
	{
		m_VMThread = CreateThread( 0, 0, StartVMThread, this, 0, 0 );
	}

	R86_PRINT( "R86: Started VM.\n" );
}

void VirtualMachine::StartSuspendedVM( void )
{
	if ( m_VMThread == 0 )
	{
		m_VMThread = CreateThread( 0, 0, StartVMThread, this, CREATE_SUSPENDED, 0 );
	}

	R86_PRINT( "R86: Started VM (Suspended).\n" );
}

void VirtualMachine::ResumeVM( void )
{
	if ( m_VMThread != 0 ) ResumeThread( m_VMThread );

	R86_PRINT( "R86: Resumed VM.\n" );
}

void VirtualMachine::SuspendVM( void )
{
	if ( m_VMThread != 0 ) SuspendThread( m_VMThread );

	R86_PRINT( "R86: Suspended VM.\n" );
}

void VirtualMachine::StopVM( void )
{
	InterlockedIncrement( &m_StopRequested );
	SetEvent( m_VMQuitEvent );

	WaitForSingleObject( m_VMThread, INFINITE );
	CloseHandle( m_VMThread );

	ResetEvent( m_VMQuitEvent );
	m_StopRequested = 0;

	m_VMThread = 0;

	R86_PRINT( "R86: Stopped VM.\n" );
}

void VirtualMachine::LoadProgram( ProgramLine* programLines, unsigned int programSize, char* staticHeapData, unsigned int sizeofStaticHeapData )
{
	bool programLoaded = ( m_CurrentProgram != 0 );

	if ( programLoaded )
	{
		ResumeVM();
		StopVM();
	}

	if ( staticHeapData != 0 )
	{
		memcpy( m_HeapInfo->hi_HeapPtr, staticHeapData, sizeofStaticHeapData );
	}

	if ( m_DeleteProgram )
	{
		delete m_CurrentProgram;
	}

	m_DeleteProgram = true;
	m_CurrentProgram = new Program( programLines, programSize, false );

	if ( programLoaded )
	{
		StartVM();
	}
}

void VirtualMachine::LoadProgram( Program* program, unsigned int programSize, char* staticHeapData, unsigned int sizeofStaticHeapData )
{
	bool programLoaded = ( m_CurrentProgram != 0 );

	if ( programLoaded )
	{
		ResumeVM();
		StopVM();
	}

	if ( staticHeapData != 0 )
	{
		memcpy( m_HeapInfo->hi_HeapPtr, staticHeapData, sizeofStaticHeapData );
	}

	if ( m_DeleteProgram )
	{
		delete m_CurrentProgram;
	}

	m_DeleteProgram = false;
	m_CurrentProgram = program;

	if ( programLoaded )
	{
		StartVM();
	}
}

void VirtualMachine::LoadProgram( const char* fileNameR86 )
{
	bool programLoaded = ( m_CurrentProgram != 0 );

	if ( programLoaded )
	{
		ResumeVM();
		StopVM();
	}

	m_DeleteProgram = true;
	m_CurrentProgram = new Program( fileNameR86, m_HeapInfo );

	if ( m_CurrentProgram->p_Halt )
	{
		delete m_CurrentProgram;
		m_CurrentProgram = 0;
	}

	if ( programLoaded )
	{
		StartVM();
	}
}

void VirtualMachine::DispatchLoop( void )
{
	while ( !m_CurrentProgram )
	{
		// Added to not stress the CPU whenever a program is not running.
		Sleep( 50 );

		if ( m_StopRequested > 0 )
		{
			return;
		}
	}

	// Reset the registers.
	m_Registers->ClearAllRegisters();
	m_Registers->ClearAllFlags();

	// Temporary vars.
	unsigned int resultUI;
	float resultF;
	uint8_t opcode = Instructions::ASM_NOP;

	// Crude way of counting instructions.
	int instructionCount = 0;
	int insSec = 0;

	unsigned int lClock = clock();
	unsigned int bp = 0;

	// Variables that are constantly used
	unsigned int intValue = 0;

	READ_NEXT;

labelBEGIN:
	instructionCount++;
	insSec++;

#ifdef SHOW_INSTRUCTION_COUNT
	if ( insSec > 16000000 )
	{
		R86_PRINT( "R86: ~Instructions per sec: %u\n", unsigned int( float( insSec ) / ( float( clock() - lClock ) / 1000.0f ) ) );
		lClock = clock();

		insSec = 0;
	}
#endif

	// Either do this if you want to sleep or if the VM has been requested to quit.
	if ( 
#ifdef INSTRUCTIONS_PER_1MS_SLEEP
		instructionCount > INSTRUCTIONS_PER_1MS_SLEEP || 
#endif
		m_StopRequested > 0 )
	{
		if ( m_StopRequested > 0 && WaitForSingleObject( m_VMQuitEvent, 0 ) == WAIT_OBJECT_0 )  
		{															  
			opcode = Instructions::END;											  
		}															  

		Sleep( 1 );
		instructionCount = 0;
	}

	// All instructions in a switch statement
	switch ( opcode )
	{
	case Instructions::ASM_ADD:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );		

		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr + *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_AND:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) & ( *(unsigned int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_CALL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		m_Stack->Push( &m_Registers->r_InstructionPointer );
		m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;

		READ_NEXT;

	case Instructions::ASM_CMP:
		m_Registers->ClearAllFlags();

		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3FF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultUI = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultUI == 0 ) *m_Registers->r_FlagZF = 1;

		if ( resultUI > *(unsigned int*) m_ProcessorState->ps_Operand1Ptr )
		{
			*m_Registers->r_FlagCF = 1;
		}

		READ_NEXT;
	
	case Instructions::ASM_DEC:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - 1;
		READ_NEXT;
	
	case Instructions::ASM_DIV:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr / *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;
	
	case Instructions::ASM_FABS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( *(float*) m_ProcessorState->ps_Operand1Ptr < 0 ) *(float*) m_ProcessorState->ps_Operand1Ptr = -*(float*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;
	
	case Instructions::ASM_FADD:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr + *(float*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;
	
	case Instructions::ASM_FATAN:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = atanf( *(float*) m_ProcessorState->ps_Operand1Ptr );
		READ_NEXT;
	
	case Instructions::ASM_FCHS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = -*(float*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;
	
	case Instructions::ASM_FCOM:
		m_Registers->ClearAllFlags();

		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xC3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultF = *(float*) m_ProcessorState->ps_Operand1Ptr - *(float*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultF == 0.0f ) *m_Registers->r_FlagZF = 1;

		if ( *(float*) m_ProcessorState->ps_Operand2Ptr >= *(float*) m_ProcessorState->ps_Operand1Ptr && resultF < 0.0f )
		{
			*m_Registers->r_FlagOF = 1;
		}

		READ_NEXT;

	case Instructions::ASM_FCOS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = cosf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	case Instructions::ASM_FDIV:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr / *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	case Instructions::ASM_FMUL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr * *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	case Instructions::ASM_FPOW:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );

		*(float*) m_ProcessorState->ps_Operand1Ptr = powf( *(float*) m_ProcessorState->ps_Operand1Ptr, *(float*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;
	
	case Instructions::ASM_FSIN:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = sinf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	case Instructions::ASM_FSQRT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = sqrtf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	case Instructions::ASM_FSUB:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr - *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	case Instructions::ASM_FTAN:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = tanf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	case Instructions::ASM_FTOI:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = (int) ( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_IAND:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) & ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_IDIV:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) / ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_IMOD:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) % ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_IMUL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) * ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_INC:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) + 1;
		READ_NEXT;

	case Instructions::ASM_INT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		intValue = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;

		if ( intValue < 256 )
		{
			if ( m_InterruptTable[ intValue ] != 0 )
			{
				m_InterruptTable[ intValue ]( this );
			}
		}

		READ_NEXT;

	case Instructions::ASM_IOR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) | ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_IXOR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) ^ ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_JA:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 0 && *m_Registers->r_FlagZF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JAE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JB:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JBE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 1 || *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT; 

	case Instructions::ASM_JG:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 && *m_Registers->r_FlagSF == *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JGE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF != *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JLE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 || *m_Registers->r_FlagSF != *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JMP:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JNE:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JNO:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagOF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JNS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JO:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagOF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_JZ:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	case Instructions::ASM_LEA:
		// Not yet implemented.
		READ_NEXT;

	case Instructions::ASM_LGA:
		_CheckProgramLineFlags(m_ProcessorState->ps_ProgramLineFlags, 0x35, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2);
		
		*(unsigned int*)m_ProcessorState->ps_Operand1Ptr = m_ProcessorState->ps_Operand2 + m_HeapInfo->hi_StaticHeapSectionOffset;
		READ_NEXT;

	case Instructions::ASM_MOD:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) % ( *(unsigned int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_MOV:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xABF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;

	case Instructions::ASM_MUL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr * *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_NEG:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) * -1;
		READ_NEXT;

	default:
	case Instructions::ASM_NOP:
		Sleep(0);
		READ_NEXT;

	case Instructions::ASM_NOT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ~( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );
		READ_NEXT;

	case Instructions::ASM_OR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr | *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_POP:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Pop( (unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_PUSH:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x555, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Push( (unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_RET:
		//_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x00, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Pop( &m_Registers->r_InstructionPointer );
		READ_NEXT;

	case Instructions::ASM_RGET:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		if ( m_GetPixelFunction != 0 )
		{
			*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = m_GetPixelFunction( m_Registers->r_RPosX, m_Registers->r_RPosY );
		}
		
		else
		{
			*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = 0;
		}

		READ_NEXT;

	case Instructions::ASM_RPLOT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( m_PlotPixelFunction != 0 )
		{
			m_PlotPixelFunction( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr, m_Registers->r_RPosX, m_Registers->r_RPosY );
		}

		READ_NEXT;

	case Instructions::ASM_RPOS:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x33F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Registers->r_RPosX = *(int*) m_ProcessorState->ps_Operand1Ptr;
		m_Registers->r_RPosY = *(int*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;

	case Instructions::ASM_SAL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) << ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_SAR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) >> ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	case Instructions::ASM_SHL:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr << *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_SHR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr >> *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_SIF:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = (float) *(int*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;

	case Instructions::ASM_SUB:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_TEST:
		m_Registers->ClearAllFlags();

		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3FF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultUI = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr & *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultUI == 0 ) *m_Registers->r_FlagZF = 1;

		if ( resultUI & ( 1 << 31 ) )
		{
			*m_Registers->r_FlagSF = 1;
		}

		READ_NEXT;

	case Instructions::ASM_UIF:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = (float) *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;

	case Instructions::ASM_XADD:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_ProcessorState->ps_UIOperand1 = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr + *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand2Ptr = m_ProcessorState->ps_UIOperand1;

		READ_NEXT;

	case Instructions::ASM_XCHG:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_ProcessorState->ps_UIOperand1 = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand2Ptr = m_ProcessorState->ps_UIOperand1;

		READ_NEXT;

	case Instructions::ASM_XOR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ^ *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	case Instructions::ASM_ASYNCK:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Registers->ClearAllFlags();

		if ( m_CheckKeyFunction != 0 && m_CheckKeyFunction( (int) *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) )
		{
			*m_Registers->r_FlagZF = 1;
		}

		READ_NEXT;

	case Instructions::ASM_GETCH:
		// Not yet implemented
		READ_NEXT;

	case Instructions::ASM_TIME:
		// Not yet implemented
		READ_NEXT;

	case Instructions::ASM_SLEEP:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		Sleep( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_UPRINT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
			
		R86_PRINT( "%u\n", *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_FPRINT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x415, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%f\n", *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	case Instructions::ASM_IPRINT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x115, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%d\n", *(int*) m_ProcessorState->ps_Operand1Ptr );
	
		READ_NEXT;

	case Instructions::ASM_CPRINT:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x115, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%c", *(char*) m_ProcessorState->ps_Operand1Ptr );
	
		READ_NEXT;

	case Instructions::ASM_PROLOG:
		//_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x0, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		bp = *m_Registers->r_BasePointer;

		*m_Registers->r_BasePointer = *m_Registers->r_StackPointer;
		m_Stack->Push( &bp );

		READ_NEXT;

	case Instructions::ASM_EPILOG:
		//_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x0, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		bp = *m_Registers->r_BasePointer;

		m_Stack->Pop( m_Registers->r_BasePointer );
		*m_Registers->r_StackPointer = bp;
	
		READ_NEXT;

	case Instructions::ASM_RCLR:
		_CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 64, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( m_ScreenClearFunction != 0 )
		{
			m_ScreenClearFunction( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );
		}
	
		READ_NEXT;

	case Instructions::END:
		break;
	}

	return;
}

// Read the next bytecode instruction and its flags.
void VirtualMachine::ReadProgram( void )
{
	if ( m_Registers->r_InstructionPointer >= m_CurrentProgram->p_NumberOfInstructions )
	{
		m_ProcessorState->ps_ProgramLineOpcode = UINT8_MAX;
		return;
	}

	ProgramLine* pl = &m_CurrentProgram->p_ProgramLines[ m_Registers->r_InstructionPointer ];

	m_ProcessorState->ps_ProgramLineOpcode = pl->pl_Opcode;
	m_ProcessorState->ps_ProgramLineFlags = pl->pl_Flags;
	m_ProcessorState->ps_Operand1 = pl->pl_Operand1;
	m_ProcessorState->ps_Operand2 = pl->pl_Operand2;

	m_Registers->r_InstructionPointer++;
}

// Bounds checking for the heap. Scheduled to be reformatted.
void VirtualMachine::CheckAddress( void* address )
{
	if ( !( address >= m_HeapInfo->hi_HeapPtr && address < ( (unsigned char*) m_HeapInfo->hi_HeapPtr + m_HeapInfo->hi_HeapLengthInBytes - sizeof( int ) ) ) )
	{
		R86_PRINT( "R86-ERROR: Access Violation at 0x%04x (0x%04x)! Out of bounds.\n", (unsigned int) address, (int) ( (unsigned int) address - (unsigned int) m_HeapInfo->hi_HeapPtr ) ); 
		m_ProcessorState->ps_Halt = true;

		m_Registers->r_InstructionPointer = m_CurrentProgram->p_NumberOfInstructions - 1;
	}
}

// Bounds checking for registers. Not in use at this moment. Reason: performance hit.
void VirtualMachine::CheckRegister( unsigned int regNum )
{
	if ( regNum >= 131 )
	{
		R86_PRINT( "R86-ERROR: Out of index register 0x%04x!\n", regNum ); 
		m_ProcessorState->ps_Halt = true;

		m_Registers->r_InstructionPointer = m_CurrentProgram->p_NumberOfInstructions - 1;
	}
}	

// Checks the flags of the program line and make the operand pointers point to the right location in memory.
// TODO: Precalc all flag combos for massive speed boost
template <unsigned short allowedFlags>
void VirtualMachine::CheckProgramLineFlags( unsigned short flags, void*& operand1, void*& operand2, unsigned int operand1Val, unsigned int operand2Val )
{
	// Check if any of the operands use a global variable and adjust the address for that.
	if ( flags & ProgramLineFlags::OPERAND1_PROPERTY_STATIC_HEAP_SECTION )
	{
		operand1Val += m_HeapInfo->hi_StaticHeapSectionOffset;
	}

	if ( flags & ProgramLineFlags::OPERAND2_PROPERTY_STATIC_HEAP_SECTION )
	{
		operand2Val += m_HeapInfo->hi_StaticHeapSectionOffset;
	}

	// No flags? Assume operands point to raw registers.
	if ( flags == ProgramLineFlags::NO_FLAGS )
	{
		operand1 = &m_Registers->r_GeneralRegisters[ operand1Val ];
		operand2 = &m_Registers->r_GeneralRegisters[ operand2Val ];

		//CheckRegister( operand1Val );
		//CheckRegister( operand2Val );
	}

	else
	{
		// Check each individual bit.
		if ( !(( flags & ProgramLineFlags::plf_RawRegisterFlagOp1 ) & allowedFlags) )
		{
			operand1 = &m_Registers->r_GeneralRegisters[ operand1Val ];
			CheckRegister( operand1Val );
		}

		if ( ( flags & ProgramLineFlags::OPERAND1_DEREFERENCE_REGISTER ) & allowedFlags )
		{
			operand1 = &m_HeapInfo->hi_HeapPtr[ m_Registers->r_GeneralRegisters[ operand1Val ] ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND1_DEREFERENCE_HEAP ) & allowedFlags )
		{
			operand1 = &m_HeapInfo->hi_HeapPtr[ m_HeapInfo->hi_HeapPtr[ operand1Val ] ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND1_HEAP_ADDRESS ) & allowedFlags )
		{
			operand1 = &m_HeapInfo->hi_HeapPtr[ operand1Val ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND1_RAW_UNSIGNED_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_UIOperand1 = operand1Val;
			operand1 = &m_ProcessorState->ps_UIOperand1;
		}

		else if ( ( flags & ProgramLineFlags::OPERAND1_RAW_SIGNED_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_IOperand1 = *( (int*) &operand1Val );
			operand1 = &m_ProcessorState->ps_IOperand1;
		}

		else if ( ( flags & ProgramLineFlags::OPERAND1_RAW_FLOAT_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_FOperand1 = *( (float*) &operand1Val );
			operand1 = &m_ProcessorState->ps_FOperand1;
		}

		if ( !( flags & ProgramLineFlags::plf_RawRegisterFlagOp2 ) )
		{
			operand2 = &m_Registers->r_GeneralRegisters[ operand2Val ];
			//CheckRegister( operand2Val );
		}

		if ( ( flags & ProgramLineFlags::OPERAND2_DEREFERENCE_REGISTER ) & allowedFlags )
		{
			operand2 = &m_HeapInfo->hi_HeapPtr[ m_Registers->r_GeneralRegisters[ operand2Val ] ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND2_DEREFERENCE_HEAP ) & allowedFlags )
		{
			operand2 = &m_HeapInfo->hi_HeapPtr[ m_HeapInfo->hi_HeapPtr[ operand2Val ] ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND2_HEAP_ADDRESS ) & allowedFlags )
		{
			operand2 = &m_HeapInfo->hi_HeapPtr[ operand2Val ];
		}

		else if ( ( flags & ProgramLineFlags::OPERAND2_RAW_UNSIGNED_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_UIOperand2 = operand2Val;
			operand2 = &m_ProcessorState->ps_UIOperand2;
		}

		else if ( ( flags & ProgramLineFlags::OPERAND2_RAW_SIGNED_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_IOperand2 = *( (int*) &operand2Val );
			operand2 = &m_ProcessorState->ps_IOperand2;
		}

		else if ( ( flags & ProgramLineFlags::OPERAND2_RAW_FLOAT_NUMBER ) & allowedFlags )
		{
			m_ProcessorState->ps_FOperand2 = *( (float*) &operand2Val );
			operand2 = &m_ProcessorState->ps_FOperand2;
		}
	}

#ifdef HEAP_CHECKS
	// See if the operands need to be checked for bounds-correctness.
	if ( flags & ProgramLineFlags::plf_HeapCheckFlagsOp1 )
	{
		CheckAddress( operand1 );
	}

	if ( flags & ProgramLineFlags::plf_HeapCheckFlagsOp2 )
	{
		CheckAddress( operand2 );
	}
#endif
}

void VirtualMachine::ResetProcessor( void )
{
	m_Registers->ClearAllFlags();
	m_Registers->ClearAllRegisters();

	m_ProcessorState->ps_Halt = true;
}

void VirtualMachine::SetScreenClearFunction( ScreenClearFunction function )
{
	m_ScreenClearFunction = function;
}

void VirtualMachine::SetPlotPixelFunction( PlotPixelFunction function )
{
	m_PlotPixelFunction = function;
}

void VirtualMachine::SetGetPixelFunction( GetPixelFunction function )
{
	m_GetPixelFunction = function;
}

void VirtualMachine::SetCheckKeyFunction( CheckKeyFunction function )
{
	m_CheckKeyFunction = function;
}

void VirtualMachine::SetInterruptFunction( InterruptFunction function, unsigned char index )
{
	m_InterruptTable[ index ] = function;
}