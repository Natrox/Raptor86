// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

#include <memory>

#include "Defines.h"

namespace Raptor
{
	namespace r86
	{
		struct HeapInfo
		{
			HeapInfo( unsigned int lengthInBytes )
				:
			hi_HeapLengthInBytes( lengthInBytes )
			{
				hi_HeapPtr = (unsigned char*) malloc( lengthInBytes );
				hi_StaticHeapSectionOffset = 0;

				if ( hi_HeapPtr == 0 )
				{
					R86_PRINT( "R86-ERROR: Failed to allocate sufficient memory (0x%04x) for the VM heap! Please lower the heap requirements.\n", lengthInBytes );
				}
			}

			~HeapInfo( void )
			{
				free( hi_HeapPtr );
			}

			// Heap information
			unsigned char* hi_HeapPtr;
			unsigned int hi_HeapLengthInBytes;

			// Offset for the static heap section data.
			unsigned int hi_StaticHeapSectionOffset;
		};
	};
};