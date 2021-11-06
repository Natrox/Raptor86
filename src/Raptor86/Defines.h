// Sam Hardeman / Natrox (c) 2013 
// Licensed under the MIT licence, see LICENSE.

#pragma once

// Comment out to disable printing to the console.
#define PRINTING_ENABLED

#ifdef PRINTING_ENABLED
	#define R86_PRINT( x, ... ) printf( x, __VA_ARGS__ );
#else
	#define R86_PRINT( x, ... )
#endif

// Comment this if you do not want to know the number of instructions per second.
#define SHOW_INSTRUCTION_COUNT

// Comment out to disable heap checking (out-of-bounds checking).
//#define HEAP_CHECKS

// You may change this define to limit the speed of the VM.
//#define INSTRUCTIONS_PER_1MS_SLEEP 0xFFFFFFFF