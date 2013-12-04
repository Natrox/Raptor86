// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#include "Stack.h"

#include "HeapInfo.h"
#include "Registers.h"
#include "ProcessorState.h"

#include "Defines.h"

using namespace Raptor::r86;

Stack::Stack( HeapInfo* heap, Registers* reg, ProcessorState* pstate )
	:
m_HeapInfo( heap ),
m_Registers( reg ),
m_ProcessorState( pstate )
{
	*reg->r_StackPointer = heap->hi_HeapLengthInBytes - 4;
	*reg->r_BasePointer = *reg->r_StackPointer;
}

Stack::~Stack( void )
{}

void Stack::Push( unsigned int* val )
{
	*m_Registers->r_StackPointer -= sizeof( unsigned int );
	CheckStackPointer();

	memcpy( (void*) &m_HeapInfo->hi_HeapPtr[ *m_Registers->r_StackPointer ], val, sizeof( unsigned int ) );
}

void Stack::Pop( unsigned int* dest )
{
	memcpy( dest, (void*) &m_HeapInfo->hi_HeapPtr[ *m_Registers->r_StackPointer ], sizeof( unsigned int ) );
	*m_Registers->r_StackPointer += sizeof( unsigned int );

	CheckStackPointer();
}

void Stack::CheckStackPointer( void )
{
	// Magic numbers suck (1048576). Will migrate to a user-specifiable stack size.
	if ( *m_Registers->r_StackPointer < m_HeapInfo->hi_HeapLengthInBytes - 1048576 )
	{
		R86_PRINT( "Stack Overflow!\n" );
		m_ProcessorState->ps_Halt = true;
	}

	else if ( *m_Registers->r_StackPointer > m_HeapInfo->hi_HeapLengthInBytes )
	{
		R86_PRINT( "Stack Underflow & Access Violation!\n" );
		m_ProcessorState->ps_Halt = true;
	}
}
