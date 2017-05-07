/*
 * WriteQasm.cpp
 *
 *  Created on: 22.04.2017
 *      Author: mueller
 */

#include "WriteQasm.h"
#include "Disassembler.h"


void WriteQasm::Write(const vector<uint64_t>& instructions, unsigned base, const DebugInfo* info)
{	// TODO: Labels from info

	// Pass 1: scan for branch targets.
	Disasm.Addr = base;
	for (uint64_t inst : instructions)
	{	Disasm.PushInstruction(inst);
		Disasm.ScanLabels();
	}
	// Pass 2: disassemble
	Disasm.Addr = base;
	for (uint64_t inst : instructions)
	{	// Labels
		string line = Disasm.GetLabel(Disasm.Addr);
		if (line.length())
		{	WriteString(line);
			WriteChar('\n');
		}
		// Code & comments
		Disasm.PushInstruction(inst);
		line = Disasm.Disassemble();
		if (Comment)
		{	checkedfprintf(Target, "\t%-55s #", line.c_str());
			if (Comment & C_Hex)
				checkedfprintf(Target, " %04x: %016" PRIx64, Disasm.Addr - sizeof(uint64_t), inst);
			if (Comment & C_Fields)
			{	WriteChar(' ');
				WriteString(Disasm.GetFields());
			}
		} else
		{	WriteChar('\t');
			WriteString(line);
		}
		WriteChar('\n');
	}

}
