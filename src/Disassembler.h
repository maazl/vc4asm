/*
 * Disassembler.h
 *
 *  Created on: 12.11.2014
 *      Author: mueller
 */

#ifndef DISASSEMBLER_H_
#define DISASSEMBLER_H_

#include "Inst.h"

#include <vector>
#include <map>
#include <cstdio>
#include <cinttypes>

using namespace std;

class Disassembler
{public:
	FILE*       Out = NULL;
	bool        UseMOV = true;
	bool        UseFloat = true;
	bool        PrintFields = false;
	uint32_t    BaseAddr = 0;
	vector<uint64_t> Instructions;
 private:
	/// Mux accumulator names
	static const char cAcc[6][4];
	/// Read register codes
	/// cReg[B!A][regnum]
	static const char cRreg[2][64][10];
	/// Write register codes
	/// cReg[B!A][regnum]
	static const char cWreg[2][64][14];
	/// ADD ALU opcodes
	static const char cOpA[33][9];
	/// MUL ALU opcodes
	static const char cOpM[9][7];
	/// LDI opcodes
	static const char cOpL[8][7];
	/// Write condition codes
	static const char cCC[8][7];
	/// Write pack codes
	/// cPack[MUL!ADD][pack]
	static const char cPack[2][16][8];
	/// Unpack codes
	static const char cUnpack[8][5];
	/// Small immediate values
	static const char cSMI[64][7];
	/// Branch conditions
	static const char cBCC[16][7];
	/// Signaling opcodes
	static const char cOpS[14][9];
 private:
	uint32_t    Addr = 0;
	/// Ordered set of branch targets in units of BaseAddr
	map<size_t,string> Labels;
	Inst        Instruct;
	char        Code[100];
	char*       CodeAt;
	char        Comment[80];
 private:
	void append(const char* str);
	void appendf(const char* fmt, ...);
	/// Use pretty value for immediates.
	void appendImmd(value_t val);
	/// Get pack extension for ALU instruction
	/// @param mul true: MUL ALU, false: ADD ALU
	/// @pre Instruct.Sig != Inst::S_BRA
	const char* packCode(bool mul);
	void appendSource(Inst::mux mux);
	void DoADD();
	void DoMUL();
	void DoALU();
	void DoLDI();
	void DoBranch();
	void DoInstruction();
 public:
	void ScanLabels();
	void Disassemble();
};

#endif // DISASSEMBLER_H_
