// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#include "VirtualMachine.h"

#include <time.h>

using namespace Raptor::r86;

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

			vm->m_OpcodeCounter = 0;
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

	memset( m_OpcodeJumpTable, 0xffff-1, 0xffff );
	m_OpcodeCounter = 0;

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

	// Point all of the opcodes to NOP initially to ignore invalid opcodes.
	for ( unsigned int i = 0; i < 0xffff; i++ )
	{
		ADD_OPCODE( labelNOP, i );
	}

	// Add all the current instructions.
	ADD_OPCODE( labelADD, -1 );
	ADD_OPCODE( labelAND, -1 );
	ADD_OPCODE( labelASYNCK, -1 );
	ADD_OPCODE( labelCALL, -1 );
	ADD_OPCODE( labelCMP, -1 );
	ADD_OPCODE( labelCPRINT, -1 );
	ADD_OPCODE( labelDEC, -1 );
	ADD_OPCODE( labelDIV, -1 );
	ADD_OPCODE( labelEPILOG, -1 );
	ADD_OPCODE( labelFABS, -1 );
	ADD_OPCODE( labelFADD, -1 );
	ADD_OPCODE( labelFATAN, -1 );
	ADD_OPCODE( labelFCHS, -1 );
	ADD_OPCODE( labelFCOM, -1 );
	ADD_OPCODE( labelFCOS, -1 );
	ADD_OPCODE( labelFDIV, -1 );
	ADD_OPCODE( labelFMUL, -1 );
	ADD_OPCODE( labelFPOW, -1 );
	ADD_OPCODE( labelFPRINT, -1 );
	ADD_OPCODE( labelFSIN, -1 );
	ADD_OPCODE( labelFSQRT, -1 );
	ADD_OPCODE( labelFSUB, -1 );
	ADD_OPCODE( labelFTAN, -1 );
	ADD_OPCODE( labelFTOI, -1 );
	ADD_OPCODE( labelGETCH, -1 );
	ADD_OPCODE( labelIAND, -1 );
	ADD_OPCODE( labelIDIV, -1 );
	ADD_OPCODE( labelIMOD, -1 );
	ADD_OPCODE( labelIMUL, -1 );
	ADD_OPCODE( labelINC, -1 );
	ADD_OPCODE( labelINT, -1 );
	ADD_OPCODE( labelIOR, -1 );
	ADD_OPCODE( labelIPRINT, -1 );
	ADD_OPCODE( labelIXOR, -1 );
	ADD_OPCODE( labelJA, -1 );
	ADD_OPCODE( labelJAE, -1 );
	ADD_OPCODE( labelJB, -1 );
	ADD_OPCODE( labelJBE, -1 );
	ADD_OPCODE( labelJE, -1 );
	ADD_OPCODE( labelJG, -1 );
	ADD_OPCODE( labelJGE, -1 );
	ADD_OPCODE( labelJL, -1 );
	ADD_OPCODE( labelJLE, -1 );
	ADD_OPCODE( labelJMP, -1 );
	ADD_OPCODE( labelJNE, -1 );
	ADD_OPCODE( labelJNO, -1 );
	ADD_OPCODE( labelJNS, -1 );
	ADD_OPCODE( labelJO, -1 );
	ADD_OPCODE( labelJS, -1 );
	ADD_OPCODE( labelJZ, -1 );
	ADD_OPCODE( labelLEA, -1 );
	ADD_OPCODE( labelMOD, -1 );
	ADD_OPCODE( labelMOV, -1 );
	ADD_OPCODE( labelMUL, -1 );
	ADD_OPCODE( labelNEG, -1 );
	ADD_OPCODE( labelNOP, -1 );
	ADD_OPCODE( labelNOT, -1 );
	ADD_OPCODE( labelOR, -1 );
	ADD_OPCODE( labelPOP, -1 );
	ADD_OPCODE( labelPROLOG, -1 );
	ADD_OPCODE( labelPUSH, -1 );
	ADD_OPCODE( labelRCLR, -1 );
	ADD_OPCODE( labelRET, -1 );
	ADD_OPCODE( labelRGET, -1 );
	ADD_OPCODE( labelRPLOT, -1 );
	ADD_OPCODE( labelRPOS, -1 );
	ADD_OPCODE( labelSAL, -1 );
	ADD_OPCODE( labelSAR, -1 );
	ADD_OPCODE( labelSHL, -1 );
	ADD_OPCODE( labelSHR, -1 );
	ADD_OPCODE( labelSIF, -1 );
	ADD_OPCODE( labelSLEEP, -1 );
	ADD_OPCODE( labelSUB, -1 );
	ADD_OPCODE( labelTEST, -1 );
	ADD_OPCODE( labelTIME, -1 );
	ADD_OPCODE( labelUIF, -1 );
	ADD_OPCODE( labelUPRINT, -1 );
	ADD_OPCODE( labelXADD, -1 );
	ADD_OPCODE( labelXCHG, -1 );
	ADD_OPCODE( labelXOR, -1 );

	ADD_OPCODE( labelEND, 0xffff - 1 );

	// Temporary vars.
	unsigned int resultUI;
	float resultF;
	void* opcodeLabel;

	// Crude way of counting instructions.
	int instructionCount = 0;
	int insSec = 0;

	unsigned int lClock = clock();

	unsigned int bp = 0;

	READ_NEXT;

labelBEGIN:
	instructionCount++;
	insSec++;

#ifdef SHOW_INSTRUCTION_COUNT
	if ( insSec > 100000000 )
	{
		R86_PRINT( "R86: ~Instructions per sec: %u\n", unsigned int( float( insSec ) / ( float( clock() - lClock ) / 1000.0f ) ) );
		lClock = clock();

		insSec = 0;
	}
#endif

	// Either do this if you want to sleep or if the VM has been requested to quit.
	if ( instructionCount > INSTRUCTIONS_PER_1MS_SLEEP || m_StopRequested > 0 )
	{
		if ( m_StopRequested > 0 && WaitForSingleObject( m_VMQuitEvent, 0 ) == WAIT_OBJECT_0 )  
		{															  
			goto labelEND;											  
		}															  

		Sleep( 1 );
		instructionCount = 0;
	}

	_asm { jmp opcodeLabel }; 		

	// Function table. See DOCUMENTATION for what these do.
	while ( 1 )
	{
	labelADD:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );		

		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr + *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelAND:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) & ( *(unsigned int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelCALL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		m_Stack->Push( &m_Registers->r_InstructionPointer );
		m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;

		READ_NEXT;

	labelCMP:
		m_Registers->ClearAllFlags();

		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3FF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultUI = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultUI == 0 ) *m_Registers->r_FlagZF = 1;

		if ( resultUI > *(unsigned int*) m_ProcessorState->ps_Operand1Ptr )
		{
			*m_Registers->r_FlagCF = 1;
		}

		READ_NEXT;
	
	labelDEC:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - 1;
		READ_NEXT;
	
	labelDIV:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr / *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;
	
	labelFABS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( *(float*) m_ProcessorState->ps_Operand1Ptr < 0 ) *(float*) m_ProcessorState->ps_Operand1Ptr = -*(float*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;
	
	labelFADD:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr + *(float*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;
	
	labelFATAN:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = atanf( *(float*) m_ProcessorState->ps_Operand1Ptr );
		READ_NEXT;
	
	labelFCHS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = -*(float*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;
	
	labelFCOM:
		m_Registers->ClearAllFlags();

		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xC3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultF = *(float*) m_ProcessorState->ps_Operand1Ptr - *(float*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultF == 0.0f ) *m_Registers->r_FlagZF = 1;

		if ( *(float*) m_ProcessorState->ps_Operand2Ptr >= *(float*) m_ProcessorState->ps_Operand1Ptr && resultF < 0.0f )
		{
			*m_Registers->r_FlagOF = 1;
		}

		READ_NEXT;

	labelFCOS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = cosf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	labelFDIV:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr / *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	labelFMUL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr * *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	labelFPOW:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );

		*(float*) m_ProcessorState->ps_Operand1Ptr = powf( *(float*) m_ProcessorState->ps_Operand1Ptr, *(float*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;
	
	labelFSIN:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = sinf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	labelFSQRT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = sqrtf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	labelFSUB:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x83F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = *(float*) m_ProcessorState->ps_Operand1Ptr - *(float*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;
	
	labelFTAN:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = tanf( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;
	
	labelFTOI:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = (int) ( *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelIAND:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) & ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelIDIV:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) / ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelIMOD:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) % ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelIMUL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) * ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelINC:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) + 1;
		READ_NEXT;

	labelINT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		unsigned int intValue = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;

		if ( intValue < 256 )
		{
			if ( m_InterruptTable[ intValue ] != 0 )
			{
				m_InterruptTable[ intValue ]( this );
			}
		}

		READ_NEXT;

	labelIOR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) | ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelIXOR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) ^ ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelJA:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 0 && *m_Registers->r_FlagZF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJAE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJB:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJBE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagCF == 1 || *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT; 

	labelJG:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 && *m_Registers->r_FlagSF == *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJGE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF != *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJLE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 || *m_Registers->r_FlagSF != *m_Registers->r_FlagOF ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJMP:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJNE:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJNO:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagOF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJNS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == 0 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJO:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagOF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagSF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelJZ:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x40, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		if ( *m_Registers->r_FlagZF == 1 ) m_Registers->r_InstructionPointer = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr; 
		READ_NEXT;

	labelLEA:
		// Not yet implemented.
		READ_NEXT;

	labelMOD:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) % ( *(unsigned int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelMOV:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xABF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;

	labelMUL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr * *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelNEG:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) * -1;
		READ_NEXT;

	labelNOP:
		_asm{ nop };
		READ_NEXT;

	labelNOT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = ~( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );
		READ_NEXT;

	labelOR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr | *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelPOP:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Pop( (unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelPUSH:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Push( (unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelRET:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x00, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Stack->Pop( &m_Registers->r_InstructionPointer );
		READ_NEXT;

	labelRGET:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );	

		if ( m_GetPixelFunction != 0 )
		{
			*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = m_GetPixelFunction( m_Registers->r_RPosX, m_Registers->r_RPosY );
		}
		
		else
		{
			*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = 0;
		}

		READ_NEXT;

	labelRPLOT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( m_PlotPixelFunction != 0 )
		{
			m_PlotPixelFunction( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr, m_Registers->r_RPosX, m_Registers->r_RPosY );
		}

		READ_NEXT;

	labelRPOS:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x33F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Registers->r_RPosX = *(int*) m_ProcessorState->ps_Operand1Ptr;
		m_Registers->r_RPosY = *(int*) m_ProcessorState->ps_Operand2Ptr;

		READ_NEXT;

	labelSAL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) << ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelSAR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x23F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(int*) m_ProcessorState->ps_Operand1Ptr = ( *(int*) m_ProcessorState->ps_Operand1Ptr ) >> ( *(int*) m_ProcessorState->ps_Operand2Ptr );
		READ_NEXT;

	labelSHL:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr << *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelSHR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr >> *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelSIF:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = (float) *(int*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;

	labelSUB:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr - *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelTEST:
		m_Registers->ClearAllFlags();

		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3FF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		resultUI = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr & *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;

		if ( resultUI == 0 ) *m_Registers->r_FlagZF = 1;

		if ( resultUI & ( 1 << 31 ) )
		{
			*m_Registers->r_FlagSF = 1;
		}

		READ_NEXT;

	labelUIF:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x15, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(float*) m_ProcessorState->ps_Operand1Ptr = (float) *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		READ_NEXT;

	labelXADD:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_ProcessorState->ps_UIOperand1 = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr + *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand2Ptr = m_ProcessorState->ps_UIOperand1;

		READ_NEXT;

	labelXCHG:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x3F, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_ProcessorState->ps_UIOperand1 = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		*(unsigned int*) m_ProcessorState->ps_Operand2Ptr = m_ProcessorState->ps_UIOperand1;

		READ_NEXT;

	labelXOR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0xBF, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		*(unsigned int*) m_ProcessorState->ps_Operand1Ptr = *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ^ *(unsigned int*) m_ProcessorState->ps_Operand2Ptr;
		READ_NEXT;

	labelASYNCK:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		m_Registers->ClearAllFlags();

		if ( m_CheckKeyFunction != 0 && m_CheckKeyFunction( (int) *(unsigned int*) m_ProcessorState->ps_Operand1Ptr ) )
		{
			*m_Registers->r_FlagZF = 1;
		}

		READ_NEXT;

	labelGETCH:
		// Not yet implemented
		READ_NEXT;

	labelTIME:
		// Not yet implemented
		READ_NEXT;

	labelSLEEP:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		Sleep( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelUPRINT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x55, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
			
		R86_PRINT( "%u\n", *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelFPRINT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x415, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%f\n", *(float*) m_ProcessorState->ps_Operand1Ptr );

		READ_NEXT;

	labelIPRINT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x115, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%d\n", *(int*) m_ProcessorState->ps_Operand1Ptr );
	
		READ_NEXT;

	labelCPRINT:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x115, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		R86_PRINT( "%c", *(char*) m_ProcessorState->ps_Operand1Ptr );
	
		READ_NEXT;

	labelPROLOG:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x0, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		bp = *m_Registers->r_BasePointer;

		*m_Registers->r_BasePointer = *m_Registers->r_StackPointer;
		m_Stack->Push( &bp );

		READ_NEXT;

	labelEPILOG:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 0x0, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		bp = *m_Registers->r_BasePointer;

		m_Stack->Pop( m_Registers->r_BasePointer );
		*m_Registers->r_StackPointer = bp;
	
		READ_NEXT;

	labelRCLR:
		CheckProgramLineFlags( m_ProcessorState->ps_ProgramLineFlags, 64, m_ProcessorState->ps_Operand1Ptr, m_ProcessorState->ps_Operand2Ptr, m_ProcessorState->ps_Operand1, m_ProcessorState->ps_Operand2 );
		
		if ( m_ScreenClearFunction != 0 )
		{
			m_ScreenClearFunction( *(unsigned int*) m_ProcessorState->ps_Operand1Ptr );
		}
	
		READ_NEXT;
	}

	labelEND:

	return;
}

// Read the next bytecode instruction and its flags.
void VirtualMachine::ReadProgram( void )
{
	if ( m_Registers->r_InstructionPointer > m_CurrentProgram->p_NumberOfInstructions )
	{
		m_ProcessorState->ps_ProgramLineOpcode = 0xffff - 1;
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
void VirtualMachine::CheckProgramLineFlags( unsigned short flags, unsigned short allowedFlags, void*& operand1, void*& operand2, unsigned int operand1Val, unsigned int operand2Val )
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
		if ( !( flags & ProgramLineFlags::plf_RawRegisterFlagOp1 ) & allowedFlags )
		{
			operand1 = &m_Registers->r_GeneralRegisters[ operand1Val ];
			//CheckRegister( operand1Val );
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

	// See if the operands need to be checked for bounds-correctness.
	if ( flags & ProgramLineFlags::plf_HeapCheckFlagsOp1 )
	{
		CheckAddress( operand1 );
	}

	if ( flags & ProgramLineFlags::plf_HeapCheckFlagsOp2 )
	{
		CheckAddress( operand2 );
	}
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