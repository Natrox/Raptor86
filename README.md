Raptor86
========

A toy virtual machine based on the x86-87 instruction set made in my spare time.


Documentation
=============

For unassembled versions of programs, check the "assembler" folder. The most complex one is "raytracer-buffered.rasm".

For the VM binary and compiled programs, check the "bin" folder.

For the source, check the "src" folder for a VS2012 project file and code files. 
Source for the assembler is not available at this time (it needs clean-up, A LOT).
However, just for information purposes, I've used flex and yacc to parse assembly and output machine code.

SDL has been included to create a surface as video output for the Raptor86. SDL is not tied to the project, you may use any video output system. The virtual machine can also run without video output. 

Assembly example
================

The assembly is non-standard, but resembles bare x86 assembly code.

It has a few extra things, mostly to ease writing programs.
Here are some examples:

<pre><code>; Semicolon for comments
; You can set-up global variables, they will be copied into memory at run-time.
; The assembler infers the type automatically - but will always allocate enough to hold a float/int.
$global1 = 38;
$global2 = 68.99f;
$global3 = 0;

; You can use them just like any register, just be sure to append the '$'
lea r0, $global1
add [r0], 4
mov $global2, [r0]
uif $global2

; Functions and labels can be defined by prefixing '?'.
; They can be called or jumped to globally, there is no scoping available.
?dothing:
  add $global3, 1
  
; There is obviously no memory management or memory protection, you can write to wherever you want.
; There is no execution from memory, so it's not possible to create self-modifying code.
mov r0, 0xff ; Hex is supported
add [r0], r1

; Lastly, you can affix literal number values to let the assembler know which type you want.
; This is important for specific functions
mov r0, 0.5f ; Float
fmul r0, 4.0f
ftoi r0
imul r0, -21s ; Signed integer 
</code></pre>
  
List of instructions
====================
| Instructions | Comments                                                                        |
|--------------|---------------------------------------------------------------------------------|
| ADD          |                                                                                 |
| AND          |                                                                                 |
| ASYNCK       | Similar to GetAsyncKeyState() on Windows. Requires user to register a callback. |
| CALL         |                                                                                 |
| CPRINT       | Helper instruction; prints an 8-bit char to stdout.                             |
| CMP          |                                                                                 |
| DEC          |                                                                                 |
| DIV          |                                                                                 |
| EPILOG       | Helper instruction; restores the stack pointer from the base pointer register.  |
| FABS         |                                                                                 |
| FADD         |                                                                                 |
| FATAN        |                                                                                 |
| FCHS         |                                                                                 |
| FCOM         |                                                                                 |
| FCOS         |                                                                                 |
| FDIV         |                                                                                 |
| FMUL         |                                                                                 |
| FPOW         |                                                                                 |
| FPRINT       | Helper instruction; prints a float to stdout.                                   |
| FSIN         |                                                                                 |
| FSQRT        |                                                                                 |
| FSUB         |                                                                                 |
| FTAN         |                                                                                 |
| FTOI         | Helper instruction; converts operand to signed integer in-place.                |
| GETCH        | Not implemented.                                                                |
| IAND         |                                                                                 |
| IDIV         |                                                                                 |
| IMOD         |                                                                                 |
| IMUL         |                                                                                 |
| INC          |                                                                                 |
| INT          | Calls a user-registered interrupt callback function.                            |
| IOR          |                                                                                 |
| IPRINT       | Helper instruction; prints a signed int to stdout.                              |
| IXOR         |                                                                                 |
| JA           |                                                                                 |
| JAE          |                                                                                 |
| JB           |                                                                                 |
| JBE          |                                                                                 |
| JE           |                                                                                 |
| JG           |                                                                                 |
| JGE          |                                                                                 |
| JL           |                                                                                 |
| JLE          |                                                                                 |
| JMP          |                                                                                 |
| JNE          |                                                                                 |
| JNO          |                                                                                 |
| JNS          |                                                                                 |
| JO           |                                                                                 |
| JS           |                                                                                 |
| JZ           |                                                                                 |
| LEA          | Not implemented as an instruction; expanded into equavalent instructions at assembler level. Caveat: You cannot use the first operand of the LEA instruction in the arithmetic of the second operand.|
| MOD          |                                                                                 |
| MOV          |                                                                                 |
| MUL          |                                                                                 |
| NEG          |                                                                                 |
| NOP          |                                                                                 |
| NOT          |                                                                                 |
| OR           |                                                                                 |
| POP          |                                                                                 |
| PROLOG       | Helper instruction; stores the stack pointer in the base pointer register.      |
| PUSH         |                                                                                 |
| RCLR         | Helper instruction; clears the screen.                                          |
| RET          |                                                                                 |
| RGET         | Helper instruction; gets pixel color at set coordinates.                        |
| RPLOT        | Helper instruction; plots pixel at set coordinates. Requires user callback.     |
| RPOS         | Helper instruction; stores the current screen pixel coordinates in a register.  |
| SAL          |                                                                                 |
| SAR          |                                                                                 |
| SHL          |                                                                                 |
| SHR          |                                                                                 |
| SIF          |                                                                                 |
| SLEEP        | Routes directly to Windows Sleep().                                             |
| SUB          |                                                                                 |
| TEST         |                                                                                 |
| TIME         | Not implemented.                                                                |
| UIF          | Helper instruction; converts operand to float in-place.                         |
| UPRINT       | Helper instruction; prints an unsigned int to stdout.                           |
| XADD         |                                                                                 |
| XCHG         |                                                                                 |
| XOR          |                                                                                 |
