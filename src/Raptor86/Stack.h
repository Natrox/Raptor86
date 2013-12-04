// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

namespace Raptor
{
	namespace r86
	{
		struct HeapInfo;
		struct Registers;
		struct ProcessorState;

		// Simple stack class (FILO).
		class Stack
		{
		public:
			Stack( HeapInfo* heap, Registers* reg, ProcessorState* pstate );
			~Stack( void );

		public:
			void Push( unsigned int* val );
			void Pop( unsigned int* dest );

		public:
			// Sanity checking.
			void CheckStackPointer( void );

		private:
			HeapInfo* m_HeapInfo;
			Registers* m_Registers;
			ProcessorState* m_ProcessorState;
		};
	};
};