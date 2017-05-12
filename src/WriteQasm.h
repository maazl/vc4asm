/*
 * WriteQasm.h
 *
 *  Created on: 22.04.2017
 *      Author: mueller
 */

#ifndef WRITEQASM_H_
#define WRITEQASM_H_

#include "utils.h"
#include "WriterBase.h"

#include <vector>
#include <cstdio>
#include <cinttypes>

using namespace std;

class Disassembler;
class Message;
class DebugInfo;

/// Worker class for the disassembler.
class WriteQasm : public WriterBase
{public:
  /// Options for output.
	enum comments : unsigned char
	{	C_NONE,         ///< None of the options below.
		C_Hex,          ///< Create comment with raw binary.
		C_Fields        ///< Write comment with decoded instruction fields. See \ref Disassembler::GetFields().
	};
	CLASSFLAGSENUM(comments, unsigned char);
 private:
	/// Disassembler to use.
	Disassembler&     Disasm;
  /// Generator options for comments.
	const comments    Comment;
	/// Flag whether the current instruction is likely not reasonable. Set by Disassembler message callback.
	bool              SuspiciousInstruction;
 public:
	/// Create QPU assembler writer.
	/// @param target Target file.
	/// @param disasm Configured disassembler.
	/// @param opt Write comments. See \ref comments.
	WriteQasm(FILE* target, Disassembler& disasm, comments opt = C_NONE);
	/// Disassemble instructions.
	/// @param instructions Code words to disassemble.
	/// @param base Base address of the instructions.
	/// @param Debug info, optional. Used for meaningful labels and data segments.
	void Write(const vector<uint64_t>& instructions, unsigned base = 0, const DebugInfo* info = NULL);
};

#endif // DISASSEMBLER_H_
