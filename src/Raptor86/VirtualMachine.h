// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

// Not compilable with x64 compilers!

#pragma once

#include <Windows.h>

#include "ProcessorState.h"
#include "Instructions.h"
#include "Registers.h"
#include "HeapInfo.h"
#include "Program.h"
#include "Stack.h"

#include "Defines.h"

// This define is used internally to add new instructions to the jumptable.
#define ADD_OPCODE( label, n ) \
	{												             \
		void* ptrr = 0;								             \
													             \
		__asm{ push eax };							             \
		__asm{ mov eax, ##label };					             \
		__asm{ mov ptrr, eax };						             \
		__asm{ pop eax };							             \
													             \
		if ( n < 0 ) m_OpcodeJumpTable[ m_OpcodeCounter++ ] = ptrr; \
		if ( n >= 0 ) { m_OpcodeJumpTable[n] = ptrr; };               \
	}									   
									
// This is a define that performs the required steps to go to the next
// instruction in the bytecode program.
#define READ_NEXT \
	ReadProgram();												  \
	opcodeLabel = m_OpcodeJumpTable[ m_ProcessorState->ps_ProgramLineOpcode ]; \
	goto labelBEGIN;							 

// Type defines for various function pointers.
typedef bool ( *CheckKeyFunction )( int );
typedef unsigned int ( *GetPixelFunction )( int, int );
typedef void ( *PlotPixelFunction )( unsigned int, int, int );
typedef void ( *ScreenClearFunction )( unsigned int );

namespace Raptor
{
	namespace r86
	{
		// These are the available preset memory sizes.
		// Feel free to expand this with your own.
		namespace HeapMemorySizes
		{
			enum HeapMemorySize
			{
				HEAP_MEMORY_64K = 0x10000,
				HEAP_MEMORY_128K = 0x20000,
				HEAP_MEMORY_256K = 0x40000,
				HEAP_MEMORY_512K = 0x80000,
				HEAP_MEMORY_1M = 0x100000,
				HEAP_MEMORY_2M = 0x200000,
				HEAP_MEMORY_4M = 0x400000,
				HEAP_MEMORY_8M = 0x800000,
				HEAP_MEMORY_16M = 0x1000000,
				HEAP_MEMORY_32M = 0x2000000,
				HEAP_MEMORY_64M = 0x4000000,
				HEAP_MEMORY_128M = 0x8000000,
				HEAP_MEMORY_256M = 0x10000000,
				HEAP_MEMORY_512M = 0x20000000,
				HEAP_MEMORY_1024M = 0x40000000
			};
		};

		// Prototype of the thread start function.
		DWORD WINAPI StartVMThread( void* clientPtr );

		class VirtualMachine;

		// Type define for custom, user-specified interrupt functions.
		typedef void ( *InterruptFunction )( VirtualMachine* );

		// The main class for virtual machine logic.
		class VirtualMachine
		{
		public:
			VirtualMachine( HeapMemorySizes::HeapMemorySize heapSize );
			~VirtualMachine( void );

		public:
			// Control options.
			void StartVM( void );
			void StartSuspendedVM( void );
			void ResumeVM( void );
			void SuspendVM( void );
			void StopVM( void );

		public:
			// Different methods of loading programs.
			void LoadProgram( ProgramLine* programLines, unsigned int programSize, char* staticHeapData = 0, unsigned int sizeofStaticHeapData = 0 );
			void LoadProgram( Program* program, unsigned int programSize, char* staticHeapData = 0, unsigned int sizeofStaticHeapData = 0 );
			void LoadProgram( const char* fileNameR86 );

		private:
			// Internal functions
			void DispatchLoop( void ); 
			void ReadProgram( void );

			void CheckAddress( void* address );
			void CheckRegister( unsigned int regNum );
			void CheckProgramLineFlags( unsigned short flags, unsigned short allowedFlags, void*& operand1, void*& operand2, unsigned int operand1Val, unsigned int operand2Val );

		private:
			void ResetProcessor( void );

		public:
			// User-specifiable functions. Completely optional for functioning.
			void SetScreenClearFunction( ScreenClearFunction function );
			void SetPlotPixelFunction( PlotPixelFunction function );
			void SetGetPixelFunction( GetPixelFunction function );
			void SetCheckKeyFunction( CheckKeyFunction function );

		public:
			// User-specifiable interrupt functions.
			void SetInterruptFunction( InterruptFunction function, unsigned char index );

		public:
			ProcessorState* m_ProcessorState;
			Registers* m_Registers;
			HeapInfo* m_HeapInfo;
			Stack* m_Stack;

		private:
			Program* m_CurrentProgram;
			bool m_DeleteProgram;

		private:
			void* m_OpcodeJumpTable[0xffff];
			unsigned int m_OpcodeCounter;

		private:
			friend DWORD WINAPI StartVMThread( void* clientPtr );
			HANDLE m_VMThread;
			HANDLE m_VMQuitEvent;

		private:
			volatile unsigned long long m_StopRequested;

		private:
			PlotPixelFunction m_PlotPixelFunction;
			ScreenClearFunction m_ScreenClearFunction;
			GetPixelFunction m_GetPixelFunction;
			CheckKeyFunction m_CheckKeyFunction;

		private:
			InterruptFunction m_InterruptTable[256];
		};
	};
}