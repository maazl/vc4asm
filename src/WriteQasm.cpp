/*
 * WriteQasm.cpp
 *
 *  Created on: 22.04.2017
 *      Author: mueller
 */

#include "WriteQasm.h"
#include "Disassembler.h"


WriteQasm::WriteQasm(FILE* target, Disassembler& disasm, comments opt)
:	WriterBase(target), Disasm(disasm), Comment(opt)
{	Disasm.OnMessage = [this](const Message& msg) { /*fputs(msg.c_str(), stderr); fputc('\n', stderr);*/ if (msg.ID.Severity() >= WARN) SuspiciousInstruction = true; };
}

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
		SuspiciousInstruction = false;
		Disasm.PushInstruction(inst);
		line = Disasm.Disassemble();
		if (SuspiciousInstruction)
		{	// Can't disassemble reasonably, write raw data field
			checkedfprintf(Target, "\t.long 0x%016" PRIx64, inst);
		} else if (Comment)
		{	checkedfprintf(Target, "\t%-55s #", line.c_str());
			if (Comment & C_Hex)
				checkedfprintf(Target, " %04x: %016" PRIx64, Disasm.Addr - (unsigned)sizeof(uint64_t), inst);
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
