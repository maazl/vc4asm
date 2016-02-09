/*
 * Disassembler.h
 *
 *  Created on: 12.11.2014
 *      Author: mueller
 */

#ifndef DISASSEMBLER_H_
#define DISASSEMBLER_H_

#include "Inst.h"
#include "utils.h"

#include <vector>
#include <map>
#include <cstdio>
#include <cinttypes>

using namespace std;

/// Worker class for the disassembler.
class Disassembler
{public:
	/// Target stream, receives the result.
	FILE*       Out = NULL;
	/// Enable the use of the \c mov pseudo instruction where possible. Default \c true.
	bool        UseMOV = true;
	/// @brief Try to identify floating point constants. Default \c true.
	/// @details This is just a guess basically the exponent is checked to be approximately in the range E-6 to E+6.
	bool        UseFloat = true;
	/// Add comment with PC and binary code to the assembler output. Default \c false.
	bool        PrintComment = false;
	/// Add additional comment with all individual fields of the QPU instruction decoded.
	/// This is mainly for testing purposes. Default \c false.
	bool        PrintFields = false;
	/// Base address, used for PC in comments and for branch target resolution. 0 by default.
	uint32_t    BaseAddr = 0;
	/// Instructions to disassemble.
	vector<uint64_t> Instructions;
 private: // Decode tables
	/// Read register codes, cReg[B!A][regnum]
	static const char cRreg[2][64][10];
	/// Write register codes, cReg[B!A][regnum]
	static const char cWreg[2][64][14];
	/// ADD ALU opcodes
	static const char cOpA[33][9];
	/// MUL ALU opcodes
	static const char cOpM[9][7];
	/// LDI opcodes
	static const char cOpL[8][7];
	/// Write condition codes
	static const char cCC[8][7];
	/// Write pack codes, cPack[MUL!ADD][pack]
	static const char cPack[2][16][8];
	/// Unpack codes
	static const char cUnpack[8][5];
	/// Small immediate values
	static const char cSMI[64][7];
	/// Branch conditions
	static const char cBCC[16][7];
	/// Signaling opcodes
	static const char cOpS[14][10];
 private: // Working set
	/// Current PC in bytes, with offset of 3 instructions during the label scan.
	uint32_t    Addr = 0;
	/// Ordered set of branch targets in units of BaseAddr.
	map<size_t,string> Labels;
	/// Current instruction to decode.
	Inst        Instruct;
	/// Fixed buffer as target for one decoded instruction.
	char        Code[100];
	/// Pointer to first unused byte in \ref Code.
	char*       CodeAt;
	/// Fixed buffer for code comments.
	char        Comment[100];
 private:
	/// Append string to \ref Code.
	void append(const char* str);
	/// Formatted append to \ref Code.
	void appendf(const char* fmt, ...) PRINTFATTR(2);

	/// @param Append immediate value to \ref Code.
	/// @param val Value to append.
	/// @details The function chooses between integer, floating point and hex format automatically.
	/// It just guesses the best format. The floating point format can be disabled by the UseFloat option.
	void appendImmd(qpuValue val);
	/// Append per element signed value.
	/// @param sign true: per element signed, false: per element unsigned.
	/// @details The immediate value is directly taken from Instruct.Immd.
	/// @pre Instruct.Sig < Inst::S_LDI
	void appendPE(bool sign);
	/// @brief Append pack extension for ALU instruction to \ref Code if any.
	/// @param mul true: MUL ALU, false: ADD ALU
	/// @pre Instruct.Sig != Inst::S_BRANCH
	/// @details The function fetches the pack mode directly from \ref Instruct.
	void appendPack(bool mul);
	/// @brief Append ALU source argument to \ref Code.
	/// @param mux Input multiplexer value.
	/// @details If the multiplexer addresses one of the register files
	/// the appropriate register number is directly taken from \ref Instruct.
	/// It also takes care of small immediate values.
	/// @pre Instruct.Sig < Inst::S_LDI
	void appendSource(Inst::mux mux);
	/// Same as appendSource but also add vector rotation if any.
	void appendMULSource(Inst::mux mux);
	/// Handle ADD ALU opcode
	/// @pre Instruct.Sig < Inst::S_LDI
	void DoADD();
	/// Handle MUL ALU opcode
	/// @pre Instruct.Sig < Inst::S_LDI
	void DoMUL();
	/// Handle ALU instruction
	/// @pre Instruct.Sig < Inst::S_LDI
	void DoALU();
	/// Handle load immediate and semaphore instructions.
	/// @pre Instruct.Sig == Inst::S_LDI
	void DoLDI();
	/// Handle branch instruction
	/// @pre Instruct.Sig == Inst::S_BRANCH
	void DoBranch();
	/// Disassemble the current instruction.
	void DoInstruction();
 public:
	/// @brief Scan the binary code for branch targets.
	/// @details This populates the labels array and should be done before the call to Disassemble.
	void ScanLabels();
	/// Disassemble the instruction words in \ref Instructions.
	void Disassemble();
};

#endif // DISASSEMBLER_H_
