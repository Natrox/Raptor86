#pragma once
// Macros for all the possibe combinations of flags with what is allowed, enjoy.
// Sources from available programs.

// Variant 191
#define VARIANT_BF(opcode, content) \
	VARIANT(opcode, 128, 191, content) \
	VARIANT(opcode, 144, 191, content) \
	VARIANT(opcode, 32, 191, content) \
	VARIANT(opcode, 16, 191, content) \
	VARIANT(opcode, 3, 191, content) \
	VARIANT(opcode, 48, 191, content) \
	VARIANT(opcode, 129, 191, content) \
	VARIANT(opcode, 1, 191, content) \
	VARIANT(opcode, 2, 191, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 191, content)
// Variant 64
#define VARIANT_40(opcode, content) \
	VARIANT(opcode, 64, 64, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 64, content)
// Variant 1023
#define VARIANT_3FF(opcode, content) \
	VARIANT(opcode, 64, 1023, content) \
	VARIANT(opcode, 128, 1023, content) \
	VARIANT(opcode, 144, 1023, content) \
	VARIANT(opcode, 32, 1023, content) \
	VARIANT(opcode, 16, 1023, content) \
	VARIANT(opcode, 3, 1023, content) \
	VARIANT(opcode, 48, 1023, content) \
	VARIANT(opcode, 512, 1023, content) \
	VARIANT(opcode, 129, 1023, content) \
	VARIANT(opcode, 1, 1023, content) \
	VARIANT(opcode, 2, 1023, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 1023, content)
// Variant 21
#define VARIANT_15(opcode, content) \
	VARIANT(opcode, 16, 21, content) \
	VARIANT(opcode, 3, 21, content) \
	VARIANT(opcode, 1, 21, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 21, content)
// Variant 2111
#define VARIANT_83F(opcode, content) \
	VARIANT(opcode, 144, 2111, content) \
	VARIANT(opcode, 32, 2111, content) \
	VARIANT(opcode, 16, 2111, content) \
	VARIANT(opcode, 3, 2111, content) \
	VARIANT(opcode, 48, 2111, content) \
	VARIANT(opcode, 2048, 2111, content) \
	VARIANT(opcode, 129, 2111, content) \
	VARIANT(opcode, 1, 2111, content) \
	VARIANT(opcode, 2, 2111, content) \
	VARIANT(opcode, 2049, 2111, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 2111, content)
// Variant 3135
#define VARIANT_C3F(opcode, content) \
	VARIANT(opcode, 144, 3135, content) \
	VARIANT(opcode, 32, 3135, content) \
	VARIANT(opcode, 16, 3135, content) \
	VARIANT(opcode, 3, 3135, content) \
	VARIANT(opcode, 48, 3135, content) \
	VARIANT(opcode, 2048, 3135, content) \
	VARIANT(opcode, 129, 3135, content) \
	VARIANT(opcode, 1, 3135, content) \
	VARIANT(opcode, 2, 3135, content) \
	VARIANT(opcode, 2049, 3135, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 3135, content)
// Variant 575
#define VARIANT_23F(opcode, content) \
	VARIANT(opcode, 144, 575, content) \
	VARIANT(opcode, 32, 575, content) \
	VARIANT(opcode, 16, 575, content) \
	VARIANT(opcode, 3, 575, content) \
	VARIANT(opcode, 48, 575, content) \
	VARIANT(opcode, 512, 575, content) \
	VARIANT(opcode, 129, 575, content) \
	VARIANT(opcode, 1, 575, content) \
	VARIANT(opcode, 2, 575, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 575, content)
// Variant 53
#define VARIANT_35(opcode, content) \
	VARIANT(opcode, 32, 53, content) \
	VARIANT(opcode, 16, 53, content) \
	VARIANT(opcode, 3, 53, content) \
	VARIANT(opcode, 48, 53, content) \
	VARIANT(opcode, 1, 53, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 53, content)
// Variant 2751
#define VARIANT_ABF(opcode, content) \
	VARIANT(opcode, 128, 2751, content) \
	VARIANT(opcode, 144, 2751, content) \
	VARIANT(opcode, 32, 2751, content) \
	VARIANT(opcode, 16, 2751, content) \
	VARIANT(opcode, 3, 2751, content) \
	VARIANT(opcode, 48, 2751, content) \
	VARIANT(opcode, 512, 2751, content) \
	VARIANT(opcode, 2048, 2751, content) \
	VARIANT(opcode, 129, 2751, content) \
	VARIANT(opcode, 1, 2751, content) \
	VARIANT(opcode, 2, 2751, content) \
	VARIANT(opcode, 2049, 2751, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 2751, content)
// Variant 1365
#define VARIANT_555(opcode, content) \
	VARIANT(opcode, 64, 1365, content) \
	VARIANT(opcode, 144, 1365, content) \
	VARIANT(opcode, 16, 1365, content) \
	VARIANT(opcode, 3, 1365, content) \
	VARIANT(opcode, 48, 1365, content) \
	VARIANT(opcode, 129, 1365, content) \
	VARIANT(opcode, 1, 1365, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 1365, content)
// Variant 85
#define VARIANT_55(opcode, content) \
	VARIANT(opcode, 64, 85, content) \
	VARIANT(opcode, 16, 85, content) \
	VARIANT(opcode, 3, 85, content) \
	VARIANT(opcode, 48, 85, content) \
	VARIANT(opcode, 1, 85, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 85, content)
// Variant 831
#define VARIANT_33F(opcode, content) \
	VARIANT(opcode, 144, 831, content) \
	VARIANT(opcode, 32, 831, content) \
	VARIANT(opcode, 16, 831, content) \
	VARIANT(opcode, 3, 831, content) \
	VARIANT(opcode, 48, 831, content) \
	VARIANT(opcode, 512, 831, content) \
	VARIANT(opcode, 129, 831, content) \
	VARIANT(opcode, 1, 831, content) \
	VARIANT(opcode, 2, 831, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 831, content)
// Variant 63
#define VARIANT_3F(opcode, content) \
	VARIANT(opcode, 32, 63, content) \
	VARIANT(opcode, 16, 63, content) \
	VARIANT(opcode, 3, 63, content) \
	VARIANT(opcode, 48, 63, content) \
	VARIANT(opcode, 1, 63, content) \
	VARIANT(opcode, 2, 63, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 63, content)
// Variant 1045
#define VARIANT_415(opcode, content) \
	VARIANT(opcode, 144, 1045, content) \
	VARIANT(opcode, 16, 1045, content) \
	VARIANT(opcode, 3, 1045, content) \
	VARIANT(opcode, 48, 1045, content) \
	VARIANT(opcode, 129, 1045, content) \
	VARIANT(opcode, 1, 1045, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 1045, content)
// Variant 277
#define VARIANT_115(opcode, content) \
	VARIANT(opcode, 144, 277, content) \
	VARIANT(opcode, 16, 277, content) \
	VARIANT(opcode, 3, 277, content) \
	VARIANT(opcode, 48, 277, content) \
	VARIANT(opcode, 129, 277, content) \
	VARIANT(opcode, 1, 277, content) \
	FALLBACK(opcode, m_ProcessorState->ps_ProgramLineFlags, 277, content)