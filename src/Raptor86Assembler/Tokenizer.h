#pragma once

#include <string>
#include "Compiler.h"

static void ParseInput( std::string flexString, InputTypes::InputType type )
{
	AddData( flexString, type );
};

extern void ParseR86File( char* file );