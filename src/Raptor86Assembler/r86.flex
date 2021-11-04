/* Tokenizer for the Raptor86 Assembler */

%{
	#include <stdio.h>
	#include "Tokenizer.h"

	#define isatty

	using namespace InputTypes;

	//<OPCODE_SPECIFIED>{DEREFERECE_ARITHMETIC}/","				BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_SPECIAL_PTR );
	//LEA_INPUT													[ \t]*("lea"|"LEA")[ ]*{ID}","[ ]*"[".+"]"
%}

%option noyywrap
%option yylineno

%x COMMENT
%s OPCODE_SPECIFIED
%s LEA_SPECIFIED
%x LEA_COMMA
%s LEA_SECOND_ARGUMENT
%s LEA_INTERNAL
%x NEXT_EXPECTED

UNSIGNED_NUMBER	   											[0-9abcdefABCDEF]+
SIGNED_NUMBER	  											"-"?[0-9]+"s"
FLOAT_NUMBER 	   											"-"?[0-9]+"."[0-9]+"f"

ID				   											[a-zA-Z_.][a-zA-Z0-9_.]*
	
OPCODE 														{ID}
GLOBAL_VAR													${ID}
REGISTER		   											[rR]{UNSIGNED_NUMBER}
STACK_PTR													"sp"
BASE_PTR													"bp"
ADDRESS			   											%("0x"?{UNSIGNED_NUMBER})
LABEL			   											"?"{ID}

LEA_INSTRUCTION												[ \t]*("lea"|"LEA")
LEA_SECOND_OPERAND											"[".+"]"
LEA_ARITHMETIC												("+"|"-"|"*"){1}

DEREFERECE_GLOBAL_VAR										"["${ID}"]"
DEREFERECE_REGISTER		   									"["[rR]{UNSIGNED_NUMBER}"]"
DEREFERECE_STACK_PTR										"[""sp""]"
DEREFERECE_BASE_PTR											"[""bp""]"
DEREFERECE_ADDRESS			   								"["%"0x"?{UNSIGNED_NUMBER}"]"

LABEL_STANDALONE											{LABEL}":"("\n"|"\r")	
COMMENT_SIG													[" "\t]*";"

OPERAND														"["*({REGISTER}|{STACK_PTR}|{BASE_PTR}|{ADDRESS}|{GLOBAL_VAR})"]"*

%%

<*>^[ \t]+

<INITIAL>{COMMENT_SIG}	   									BEGIN( COMMENT );
<NEXT_EXPECTED,OPCODE_SPECIFIED>{COMMENT_SIG}	   			BEGIN( COMMENT );
<COMMENT>("\n"|"\r")										BEGIN( INITIAL );
<COMMENT>.

<NEXT_EXPECTED,OPCODE_SPECIFIED>("\n"|"\r")					BEGIN( INITIAL ); ParseInput( yytext, INPUT_NEW_LINE );

<INITIAL>[ \t]*[$]{ID}[ \t]*"="[ \t]*"-"?[0-9]+"."?[0-9]*[sf]?	ParseInput( yytext, INPUT_GVAR );
<INITIAL>[ \t]*[$]{ID}[ \t]*"="[ \t].*							ParseInput( yytext, INPUT_GVAR );

<INITIAL>{LEA_INSTRUCTION}									BEGIN( LEA_SPECIFIED ); ParseInput( yytext, INPUT_LEA_INSTRUCTION );
<INITIAL>[ \t]*{OPCODE}										BEGIN( OPCODE_SPECIFIED ); ParseInput( yytext, INPUT_INSTRUCTION );

<LEA_SPECIFIED>{DEREFERECE_GLOBAL_VAR}						BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_DF_GVAR );
<LEA_SPECIFIED>{DEREFERECE_REGISTER}						BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_DF_REGISTER );
<LEA_SPECIFIED>{DEREFERECE_STACK_PTR}						BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_DF_STACK_PTR );
<LEA_SPECIFIED>{DEREFERECE_BASE_PTR}						BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_DF_BASE_PTR );
<LEA_SPECIFIED>{DEREFERECE_ADDRESS}							BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_DF_ADDRESS );
	
<LEA_SPECIFIED>{GLOBAL_VAR}									BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_GVAR );
<LEA_SPECIFIED>{REGISTER}									BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_REGISTER );
<LEA_SPECIFIED>{STACK_PTR}									BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_STACK_PTR );
<LEA_SPECIFIED>{BASE_PTR}									BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_BASE_PTR );
<LEA_SPECIFIED>"0x"?{ADDRESS}								BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_ADDRESS );
<LEA_SPECIFIED>{LABEL}										BEGIN( LEA_COMMA );	ParseInput( yytext, INPUT_OPERAND_LABEL );

<LEA_COMMA>","[ ]*											BEGIN( LEA_SECOND_ARGUMENT );
<LEA_SECOND_ARGUMENT>"["									BEGIN( LEA_INTERNAL ); ParseInput( yytext, INPUT_LEA_INTERNAL );

<LEA_INTERNAL>"0x"?{UNSIGNED_NUMBER}						BEGIN( LEA_INTERNAL );	ParseInput( yytext, INPUT_OPERAND_UINT );
<LEA_INTERNAL>{GLOBAL_VAR}									BEGIN( LEA_INTERNAL );	ParseInput( yytext, INPUT_OPERAND_GVAR );
<LEA_INTERNAL>{REGISTER}									BEGIN( LEA_INTERNAL );	ParseInput( yytext, INPUT_OPERAND_REGISTER );
<LEA_INTERNAL>"0x"?{ADDRESS}								BEGIN( LEA_INTERNAL );	ParseInput( yytext, INPUT_OPERAND_ADDRESS );
<LEA_INTERNAL>{LEA_ARITHMETIC}								BEGIN( LEA_INTERNAL );	ParseInput( yytext, INPUT_LEA_MATH );
<LEA_INTERNAL>"]"											BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_UNKNOWN );
	
<OPCODE_SPECIFIED>"0x"?{UNSIGNED_NUMBER}					BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_UINT );
<OPCODE_SPECIFIED>{SIGNED_NUMBER}							BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_INT );
<OPCODE_SPECIFIED>{FLOAT_NUMBER}							BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_FLOAT );

<OPCODE_SPECIFIED>{DEREFERECE_GLOBAL_VAR}					BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_GVAR );
<OPCODE_SPECIFIED>{DEREFERECE_REGISTER}						BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_REGISTER );
<OPCODE_SPECIFIED>{DEREFERECE_STACK_PTR}					BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_STACK_PTR );
<OPCODE_SPECIFIED>{DEREFERECE_BASE_PTR}						BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_BASE_PTR );
<OPCODE_SPECIFIED>{DEREFERECE_ADDRESS}						BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_DF_ADDRESS );

<OPCODE_SPECIFIED>{GLOBAL_VAR}								BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_GVAR );
<OPCODE_SPECIFIED>{REGISTER}								BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_REGISTER );
<OPCODE_SPECIFIED>{STACK_PTR}								BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_STACK_PTR );
<OPCODE_SPECIFIED>{BASE_PTR}								BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_BASE_PTR );
<OPCODE_SPECIFIED>"0x"?{ADDRESS}							BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_ADDRESS );
<OPCODE_SPECIFIED>{LABEL}									BEGIN( NEXT_EXPECTED );	ParseInput( yytext, INPUT_OPERAND_LABEL );

<OPCODE_SPECIFIED>{ID}										BEGIN( OPCODE_SPECIFIED );	ParseInput( yytext, INPUT_INSTRUCTION );

<NEXT_EXPECTED>","[ ]*										BEGIN( OPCODE_SPECIFIED );

<INITIAL,NEXT_EXPECTED,OPCODE_SPECIFIED>[ \t]*{LABEL}/[":"]("\n"|"\r")* BEGIN( INITIAL ); ParseInput( yytext, INPUT_LABEL );					

[ \t\n]+     

.

%%

void ParseR86File( char* fileName )
{
	yyin = fopen( fileName, "r" );
	yylex();

	fclose( yyin );
}