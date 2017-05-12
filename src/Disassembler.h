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
#include "Message.h"

#include <unordered_map>
#include <cinttypes>

using namespace std;

/// Worker class for the disassembler.
/// @details This is a two pass process.
/// - In the first stage the code is scanned for branch targets. They are placed in \ref Labels.
/// - In the second pass the instructions are disassembled.
/// @example @code
/// vector<uint64_t> Instructions; // Should contain the instructions to decode.
/// Disassembler dis;
/// for (uint64_t inst : Instructions)
///   dis.PushInstruction(inst), dis.ScanLabels();
/// dis.Addr = 0;
/// for (uint64_t inst : Instructions)
/// 	dis.PushInstruction(inst), cout << dis.GetLabel() << '\t' << dis.Disassemble();
/// @endcode
class Disassembler
{public: // public types
	enum options : unsigned char ///< Options for disassembler.
	{	O_NONE         = 0x00 ///< None of the options below.
	,	O_UseMOV       = 0x01 ///< Enable the use of the \c mov pseudo instruction where possible. Default \c true.
	,	O_UseFloat     = 0x02 ///< Try to identify floating point constants. Default \c true. @details This is just a guess basically the exponent is checked to be approximately in the range E-6 to E+6.
	};
	CLASSFLAGSENUM(options, unsigned char);
 public: // public fields
	/// Disassembler options
	options           Options = O_UseMOV|O_UseFloat;
	/// Current PC in bytes.
	unsigned          Addr = 0;
	/// Set of branch targets in units of BaseAddr.
	unordered_multimap<unsigned,string> Labels;
	/// Handler for Disassembler messages. Normally if the current instruction cannot be decoded reliably.
	MessageHandler    OnMessage;
 public: // public functions
	/// Push instruction into the disassembler and decode it.
	/// @post \ref Addr is incremented by the size of one instruction word.
	/// So to process a stream of instructions you need not to adjust \ref Addr between calls.
	void PushInstruction(uint64_t inst);
	/// @brief Scan the currently decoded instruction for branch target and populate \ref Labels.
	/// @details The label scan should be done before any call to \ref Disassemble.
	/// If you have label definitions from another source place them in \ref Labels before any call to this method.
	/// @pre The instruction to scan should be placed with \ref PushInstruction before this call.
	/// @pre The base address of the instruction should be placed in \ref Addr before this call.
	/// @post \ref Labels is populated with detected branch targets if no matching entry already exists.
	/// @remarks Don't forget to reset \ref Addr after the label scan completed.
	void ScanLabels();
	/// If a label is defined at the given address, return a label reference or definition.
	/// @param addr Expected label value.
	/// @return Label definition or empty string.
	string GetLabel(unsigned addr) const;
	/// Disassemble one instruction.
	/// @pre The instruction to scan should be placed with \ref PushInstruction before this call.
	/// @pre Disassemble looks for defined symbols in \ref Labels when handling branch targets.
	/// @pre The base address of the instruction should be placed in \ref Addr before this call.
	/// @return Disassembled instruction word.
	/// @post \ref Addr is incremented by the size of one instruction word.
	/// So to decode a stream of instructions you need not to adjust \ref Addr between calls.
	string Disassemble();
	/// Create a string with the decoded instruction fields. This is mainly for testing purposes.
	/// @pre The instruction to scan should be placed with \ref PushInstruction before this call.
	/// @return String with all relevant instruction word field values.
	/// The exact format depends on the instruction type, i.e. branch, ldi or ALU.
	string GetFields();
 private: // Decode tables
	/// Read register codes, \c cReg[B!A][regnum]
	static const char cRreg[2][64][9];
	/// Write register codes, \c cReg[B!A][regnum]
	static const char cWreg[2][64][14];
	/// ADD ALU opcodes
	static const char cOpA[33][8];
	/// MUL ALU opcodes
	static const char cOpM[9][7];
	/// LDI opcodes
	static const char cOpL[8][7];
	/// Write condition codes
	static const char cCC[8][7];
	/// Write pack codes, \c cPack[MUL!ADD][pack&15]
	static const char cPack[2][16][8];
	/// Unpack codes
	static const char cUnpack[8][5];
	/// Pack/Unpack code extensions, \c cPUPX[pack>>Inst::P_INT]
	static const char cPUPX[4][2];
	/// Small immediate values
	static const char cSMI[64][7];
	/// Branch conditions
	static const char cBCC[16][7];
	/// Signaling opcodes
	static const char cOpS[14][10];
 private: // Working set
	/// Current instruction to decode.
	Inst        Instruct;
	/// Fixed buffer as target for one decoded instruction.
	char        Code[100];
	/// Pointer to first unused byte in \ref Code.
	char*       CodeAt;
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
	/// Create additional read instruction if unused register file access exists.
	/// @param regfile Register file to check. Inst::X_RA or Inst::X_RB.
	/// Every other value is invalid.
	/// @pre Instruct.Sig < Inst::S_LDI
	void DoRead(Inst::mux regfile);
	/// Handle ALU instruction
	/// @pre Instruct.Sig < Inst::S_LDI
	void DoALU();
	/// Handle load immediate and semaphore instructions.
	/// @pre Instruct.Sig == Inst::S_LDI
	void DoLDI();
	/// Handle branch instruction
	/// @pre Instruct.Sig == Inst::S_BRANCH
	void DoBranch();
};

#endif // DISASSEMBLER_H_
