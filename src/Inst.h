/*
 * Instruction.h
 *
 *  Created on: 25.10.2014
 *      Author: mueller
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_

#include "expr.h"

#include <cstdint>
using namespace std;


struct qpuValue
{	union
	{	uint32_t  uValue;   ///< Value as unsigned integer
		int32_t   iValue;   ///< Value as signed integer
		float     fValue;   ///< Value as 32 bit float
		uint8_t   cValue[4];///< Value as 4 separate bytes @remarks The endianess does not count here.
	};
	/// Assign from expression value if possible.
	/// @exception std::string Error during conversion.
	qpuValue& operator=(const exprValue& value);
};

/// Worker class to assemble or disassemble QPU instruction words.
struct Inst
{	/// Signaling bits
	enum sig : uint8_t
	{	S_BREAK   ///< Software Breakpoint
	,	S_NONE    ///< No Signal (default)
	,	S_THRSW   ///< Thread Switch (not last)
	,	S_THREND  ///< Program End (Thread End)
	,	S_SBWAIT  ///< Wait for Scoreboard (stall until this QPU can safely access tile buffer)
	,	S_SBDONE  ///< Scoreboard Unlock
	,	S_LTHRSW  ///< Last Thread Switch
	,	S_LOADCV  ///< Coverage load from Tile Buffer to r4
	,	S_LOADC   ///< Color Load from Tile Buffer to r4
	,	S_LDCEND  ///< Color Load and Program End
	,	S_LDTMU0  ///< Load (read) data from TMU0 to r4
	,	S_LDTMU1  ///< Load (read) data from TMU1 to r4
	,	S_LOADAM  ///< Alpha-Mask Load from Tile Buffer to r4
	,	S_SMI     ///< ALU instruction with raddr_b specifying small immediate or vector rotate
	,	S_LDI     ///< Load Immediate Instruction
	,	S_BRANCH  ///< Branch Instruction
	};
	/// ADD ALU operator
	enum opadd : uint8_t
	{	A_NOP
	,	A_FADD    ///< rd = ra + rb         (floating point addition)
	,	A_FSUB    ///< rd = ra - rb         (floating point subtraction)
	,	A_FMIN    ///< rd = fmin(ra, rb)    (floating point minimum)
	,	A_FMAX    ///< rd = fmax(ra, rb)    (floating point maximum)
	,	A_FMINABS ///< rd = fminabs(ra, rb)
	,	A_FMAXABS ///< rd = fmaxabs(ra, rb)
	,	A_FTOI    ///< rd = int(rb)         (convert float to int)
	,	A_ITOF    ///< rd = float(rb)       (convert int to float)
	,	A_ADD=12  ///< rd = ra + rb         (integer addition)
	,	A_SUB     ///< rd = ra - rb         (integer subtraction)
	,	A_SHR     ///< rd = ra >>> rb       (logical shift right)
	,	A_ASR     ///< rd = ra >> rb        (arithmetic shift right)
	,	A_ROR     ///< rd = ror(ra, rb)     (rotate right)
	,	A_SHL     ///< rd = ra << rb        (logical shift left)
	,	A_MIN     ///< rd = min(ra, rb)     (integer min)
	,	A_MAX     ///< rd = max(ra, rb)     (integer max)
	,	A_AND     ///< rd = ra & rb         (bitwise and)
	,	A_OR      ///< rd = ra | rb         (bitwise or,  note: or rd, ra, ra is used for mov)
	,	A_XOR     ///< rd = ra ^ rb         (bitwise xor)
	,	A_NOT     ///< rd = ~rb             (bitwise not)
	,	A_CLZ     ///< rd = clz(rb)         (count leading zeros)
	,	A_V8ADDS=30///<rd[i] = sat8(ra[i]+rb[i]), i = 0..3 / a..d
	,	A_V8SUBS  ///< rd[i] = sat8(ra[i]-rb[i]), i = 0..3 / a..d
	};
	/// MUL ALU operator
	enum opmul : uint8_t
	{	M_NOP
	,	M_FMUL    ///< rd = ra * rb
	,	M_MUL24
	,	M_V8MULD  ///< rd[i] = ra[i] * rb[3], i = 0..3 / a..d
	,	M_V8MIN   ///< rd[i] = min(ra[i], rb[i]), i = 0..3 / a..d, (note: v8min rd, ra, ra is used for mov)
	,	M_V8MAX   ///< rd[i] = max(ra[i], rb[i]), i = 0..3 / a..d
	,	M_V8ADDS  ///< rd[i] = sat8(ra[i] + rb[i]), i = 0..3 / a..d
	,	M_V8SUBS  ///< rd[i] = sat8(ra[i] - rb[i]), i = 0..3 / a..d
	};
	/// Branch condition
	enum condb : uint8_t
	{	B_ALLZS   ///< all zero set
	,	B_ALLZC   ///< all zero clear
	,	B_ANYZS   ///< any zero set
	,	B_ANYZC   ///< any zero clear
	,	B_ALLNS   ///< all negative set
	,	B_ALLNN   ///< all negative clear
	,	B_ANYNS   ///< any negative set
	,	B_ANYNC   ///< any negative clear
	,	B_ALLCS   ///< all carry set
	,	B_ALLCC   ///< all carry clear
	,	B_ANYCS   ///< any carry set
	,	B_ANYCC   ///< any carry clear
	,	B_AL = 15 ///< always (default)
	};
	/// ALU write condition
	enum conda : uint8_t
	{	C_NEVER   ///< Never (NB gates ALU – useful for LDI instructions to save ALU power)
	,	C_AL      ///< Always (default)
	,	C_ZS      ///< zero set
	,	C_ZC      ///< zero clear
	,	C_NS      ///< negative set
	,	C_NC      ///< negative clear
	,	C_CS      ///< carry set
	,	C_CC      ///< carry clear
	};
	/// Pack mode
	enum pack : uint8_t
	{	P_32      ///< write 32 bit = no pack
	,	P_16a     ///< write lower 16 bits only
	,	P_16b     ///< write lower 16 bits to the upper 16 bits
	,	P_8abcd   ///< replicate low byte over all 4 bytes
	,	P_8a      ///< write low byte only
	,	P_8b      ///< write low byte to bits 8 to 15
	,	P_8c      ///< write low byte to bits 16 to 23
	,	P_8d      ///< write low byte to bits 24 to 31
	,	P_32S     ///< write 32 bit with saturation
	,	P_16aS    ///< write lower 16 bits only with saturation
	,	P_16bS    ///< write lower 16 bits to the upper 16 bits with saturation
	,	P_8abcdS  ///< replicate low byte over all 4 bytes with saturation
	,	P_8aS     ///< write low byte only with saturation
	,	P_8bS     ///< write low byte to bits 8 to 15 with saturation
	,	P_8cS     ///< write low byte to bits 16 to 23 with saturation
	,	P_8dS     ///< write low byte to bits 24 to 31 with saturation
	};
	/// Unpack mode
	enum unpack : uint8_t
	{	U_32      ///< read 32 bit = no unpack
	,	U_16a     ///< read lower 16 bit
	,	U_16b     ///< read higher 16 bit
	,	U_8dr     ///< replicate bits 24 to 31 over all 4 bytes
	,	U_8a      ///< read low byte
	,	U_8b      ///< read bits 8 to 15
	,	U_8c      ///< read bits 16 to 23
	,	U_8d      ///< read bits 24 to 31
	};
	/// ALU input multiplexer values
	enum mux : uint8_t
	{	X_R0      ///< Accumulator r0
	,	X_R1      ///< Accumulator r1
	,	X_R2      ///< Accumulator r2
	,	X_R3      ///< Accumulator r3
	,	X_R4      ///< Accumulator r4
	,	X_R5      ///< Accumulator r5
	,	X_RA      ///< Register file A
	,	X_RB      ///< Register file B
	};
	/// Special named registers
	/// @remarks There are many more named registers, but none of them have a special meaning in this context.
	/// See Parser::regMap for a list of all registers.
	enum reg : uint8_t
	{	R_NOP = 39
	};
	/// ldi variant
	enum ldmode : uint8_t
	{	L_LDI = 0 ///< load (as it is)
	,	L_PES = 1 ///< load immediate per element signed
	,	L_PEU = 3 ///< load immediate per element unsigned
	,	L_SEMA = 4///< semaphore instruction
	};

	sig        Sig;     ///< Signaling bits
	bool       WS;      ///< Write swap
	uint8_t    WAddrA;  ///< Write address ADD ALU
	uint8_t    WAddrM;  ///< Write address MUL ALU
	uint8_t    RAddrA;  ///< Read address for register file A, invalid for ldi and semaphore instructions
	union
	{	struct            // ALU
		{	unpack Unpack;  ///< Unpack mode, only valid for ALU instructions
			union
			{	uint8_t RAddrB;///< Read address for register file B, only valid for ALU instruction without small immediate flag.
				uint8_t SImmd;///< Small immediate value, only valid for Sig == S_SMI.
			};
			mux    MuxAA;   ///< ADD ALU multiplexer A
			mux    MuxAB;   ///< ADD ALU multiplexer B
			mux    MuxMA;   ///< MUL ALU multiplexer A
			mux    MuxMB;   ///< MUL ALU multiplexer B
			opmul  OpM;     ///< ADD ALU instruction
			opadd  OpA;     ///< MUL ALU instruction
		};
		struct            // ldi, sema, branch
		{	ldmode LdMode;  ///< load immediate mode
			qpuValue Immd;  ///< immediate value
		};
	};
	union
	{	struct            // !branch
		{	bool   PM;      ///< MUL ALU pack/unpack
			pack   Pack;    ///< Pack mode
			conda  CondA;   ///< Write condition for ADD ALU
			conda  CondM;   ///< Write condition for MUL ALU
			bool   SF;      ///< Set flags
		};
		struct            // branch
		{	condb  CondBr;  ///< Branch condition
			bool   Rel;     ///< PC relative branch
			bool   Reg;     ///< Add register file A value to branch target
		};
	};

	/// Check whether a register read of register file A and B is interchangeable,
	/// i.e. X_RA/RAddrA and X_RB/RAddrB are both options.
	static bool isRRegAB(uint8_t reg) { return ((1ULL<<reg) & 0x0009008900000000ULL) != 0; }
	/// Check whether a register write to register file A and B is interchangeable,
	/// i.e. \ref WS is not fixed.
	static bool isWRegAB(uint8_t reg) { return ((1ULL<<reg) & 0xfff9f9df00000000ULL) != 0; }

	/// Simulate a ADD ALU operation of the current instruction.
	/// @param l Left operand and result
	/// @param r Right operand
	/// @return false: Invalid operator
	bool       evalADD(qpuValue& l, qpuValue r);
	/// Simulate a MUL ALU operation of the current instruction.
	/// @param l Left operand and result
	/// @param r Right operand
	/// @return false: Invalid operator
	bool       evalMUL(qpuValue& l, qpuValue r);
	/// Simulate pack operation of the current instruction.
	bool       evalPack(qpuValue& r, qpuValue v, bool mul);

	/// Reset instruction to its initial state, i.e. \c nop.
	void       reset();
	/// Create initial instruction, i.e. \c nop.
	Inst()     { reset(); }

	/// Semaphore access type
	/// @return true: release, false: acquire
	/// @pre Sig == S_LDI && LdMode == L_SEMA
	bool       SA() const { return (Immd.uValue & 0x10) != 0; }
	/// Semaphore number
	/// @pre Sig == S_LDI && LdMode == L_SEMA
	uint8_t    Sema() const { return Immd.uValue & 0xf; }
	/// Get small immediate value
	/// @return effective value represented by SImmd
	/// @pre Sig == S_SMI && SImmd < 48
	qpuValue   SMIValue() const;
	/// Converts an integer constant into a small immediate value.
	/// @param i Integer value
	/// @return Small immediate value or 0xff if \p i cannot be expressed by a small immediate value.
	static uint8_t AsSMIValue(qpuValue i);
	/// Check whether an input mux supports full MUL ALU vector rotation, i.e. swapping between QPU slices.
	/// @param m Input mux to check.
	/// @return true for r0 to r3 and r5.
	static bool SupportsFullRotate(mux m) { return (m ^ 1) < 5; }

	/// Check whether ADD ALU is in use
	/// @pre Sig < S_LDI
	bool       isADD() const { return WAddrA != R_NOP || OpA != Inst::A_NOP; }
	/// Check whether MUL ALU is in use
	/// @pre Sig < S_LDI
	bool       isMUL() const { return WAddrM != R_NOP || OpM != Inst::M_NOP; }

	/// Flags set by ADD ALU
	/// @pre Sig < S_LDI
	bool       isSFADD() const { return SF && OpA != A_NOP && CondA != C_NEVER; }
	/// Flags set by MUL ALU
	/// @pre Sig < S_LDI
	bool       isSFMUL() const { return SF && !(OpA != A_NOP && CondA != C_NEVER); }
	/// Check whether an ADD operator is an unary operator.
	/// @pre Sig < S_LDI
	bool       isUnary() const { return (0x01800180 & (1<<OpA)) != 0; }

	/// Try to swap ADD and MUL ALU of the current instruction
	/// @param mul true: move MUL instruction to ADD ALU,
	///            false: move ADD instruction to MUL ALU
	/// @return true: swap succeeded
	bool       trySwap(bool mul);

	/// Some optimizations to save ALU power
	void       optimize();
	/// Encode instruction to QPU binary format
	uint64_t   encode() const;
	/// Decode instruction from QPU binary format into this instance.
	void       decode(uint64_t code);

	/// Convert input mux to human readable format, e.g. for error messages.
	static const char* toString(mux m);
};

#endif // INSTRUCTION_H_
