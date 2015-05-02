/*
 * Disassembler.cpp
 *
 *  Created on: 12.11.2014
 *      Author: mueller
 */

#include "Disassembler.h"
#include "utils.h"

#define __STDC_FORMAT_MACROS
#include <cstdarg>
#include <cstring>
#include <cinttypes>

#include "Disassembler.tables.cpp"


void Disassembler::append(const char* str)
{	auto len = strlen(str);
	memcpy(CodeAt, str, min((size_t)(Code + sizeof Code - CodeAt), len));
	CodeAt += len;
}
void Disassembler::appendf(const char* fmt, ...)
{	va_list va;
	va_start(va, fmt);
	CodeAt += vsnprintf(CodeAt, Code + sizeof Code - CodeAt, fmt, va);
	va_end(va);
}

void Disassembler::appendImmd(value_t value)
{	// Check whether likely a float, just a guess
	int len;
	if ( UseFloat && abs(((value.iValue >> 23) & 0xff) ^ 0x80) <= 10
		&& (len = snprintf(CodeAt, Code + sizeof Code - CodeAt, "%g", value.fValue)) <= 6 )
		CodeAt += len;
	else if (abs(value.iValue) < 256)
		appendf("%i", value.iValue);
	else
		appendf("0x%x", value.uValue);
}

void Disassembler::appendPE(uint32_t val, bool sign)
{	signed char values[16];
	if (sign)
		for (int pos = 16; pos--; val <<= 1)
			values[pos] = (((int32_t)val >> 30) & 2) | ((val >> 15) & 1);
	else
		for (int pos = 16; pos--; val <<= 1)
			values[pos] = ((val >> 30) & 2) | ((val >> 15) & 1);
	*CodeAt++ = '[';
	for (int val : values)
		appendf("%i,", val);
	CodeAt[-1] = ']';
}

const char* Disassembler::packCode(bool mul)
{	return cPack[mul][Instruct.Pack * (Instruct.PM == mul)];
}

void Disassembler::appendSource(Inst::mux mux)
{	append(", ");
	switch (mux)
	{case Inst::X_RA:
		append(cRreg[0][Instruct.RAddrA]);
		break;
	 case Inst::X_RB:
		if (Instruct.Sig == Inst::S_SMI)
		{	append(cSMI[Instruct.SImmd]);
			break;
		}
		append(cRreg[1][Instruct.RAddrB]);
		break;
	 default:
		append(cAcc[mux]);
	}
	if ( (Instruct.PM && mux == Inst::X_R4) // r4 unpack
		|| (!Instruct.PM && mux == Inst::X_RA) ) // RA unpack
		append(cUnpack[Instruct.Unpack]);
}

unsigned tmpthis=0;
unsigned tmpnext=0;
char tmpbuff[256];
#define tmpalloc(sizebytes) ( tmpthis = tmpnext+sizebytes > sizeof(tmpbuff) ? 0 : tmpnext, tmpnext = (tmpthis+sizebytes), &tmpbuff[tmpthis])

void Disassembler::DoADD()
{
	uint8_t opa = Instruct.OpA;
	bool isUnary = Instruct.isUnary();

	if ( UseMOV
		&& ( (Instruct.MuxAA == Instruct.MuxAB && (0x807c2000 & (1<<opa))) // Both inputs equal and instruction that returns identity or 0
			|| ( Instruct.Sig == Inst::S_SMI && Instruct.MuxAB == Inst::X_RB
				&& (isUnary || Instruct.MuxAA == Inst::X_RB) ))) // unary or binary operator on constant
		opa = 32;

	append(cOpA[opa]);
	if (opa != Inst::A_NOP)
	{	append(cCC[Instruct.CondA]);
		if (Instruct.isSFADD())
			append(".setf");
		append(" ");

		// Parameters for ADD ALU
		// Target
		append(cWreg[Instruct.WS][Instruct.WAddrA]);
		if (!Instruct.PM && !Instruct.WS)
			append(cPack[0][Instruct.Pack]);

		if (opa != 32 && !isUnary)
			appendSource(Instruct.MuxAA);

		if (opa == 32)
		{	switch (Instruct.OpA)
			{case Inst::A_SUB:
			 case Inst::A_XOR:
			 case Inst::A_V8SUBS:
				return append(", 0");
			 default:;
			}
			if (Instruct.Sig == Inst::S_SMI && Instruct.MuxAB == Inst::X_RB)
			{	// constant => evaluate
				value_t value(Instruct.SMIValue());
				Inst::eval(Instruct.OpA, value, value);
				append(", ");
				appendImmd(value);
				return;
			}
		}

		appendSource(Instruct.MuxAB);
	}
}

void Disassembler::DoMUL()
{
	uint8_t opm = Instruct.OpM;

	if ( UseMOV && Instruct.MuxMA == Instruct.MuxMB // Both inputs equal and
		&& ( (0xb0 & (1<<opm)) // instruction that returns identity or 0 or
			|| (Instruct.Sig == Inst::S_SMI && Instruct.MuxMB == Inst::X_RB) )) // small immediate
		opm = 8;

	if (opm != Inst::M_NOP)
	{	append("; ");
		append(cOpM[opm]);
		append(cCC[Instruct.CondM]);
		if (Instruct.isSFMUL())
			append(".setf");
		if (Instruct.Sig == Inst::S_SMI && Instruct.SImmd >= 48)
			appendf(".rot %d,", Instruct.SImmd - 48);

		append(" ");
		append(cWreg[!Instruct.WS][Instruct.WAddrM]);
		if (Instruct.PM || Instruct.WS)
			append(cPack[0][Instruct.Pack]);

		if (opm == 8)
		{	if (Instruct.OpM == Inst::M_V8SUBS)
				return append(", 0");
			if (Instruct.Sig == Inst::S_SMI && Instruct.MuxMB == Inst::X_RB)
			{	// constant => evaluate
				value_t value(Instruct.SMIValue());
				Inst::eval(Instruct.OpM, value, value);
				append(", ");
				appendImmd(value);
				return;
			}
		} else
			appendSource(Instruct.MuxMA);
		appendSource(Instruct.MuxMB);
	}
}

void Disassembler::DoALU()
{
	if (PrintFields)
		sprintf(Comment, " sig%X ra%02d rb%02d pm%d upk%d pck%X"
		                 " Aop%02d Acc%d Aw%02d Aa%d Ab%d"
		                 " Mop%d Mcc%d Mw%02d Ma%d Mb%d"
		                 " sf%d ws%d",
			Instruct.Sig, Instruct.RAddrA, Instruct.RAddrB, Instruct.PM, Instruct.Unpack, Instruct.Pack,
			Instruct.OpA, Instruct.CondA, Instruct.WAddrA, Instruct.MuxAA, Instruct.MuxAB,
			Instruct.OpM, Instruct.CondM, Instruct.WAddrM, Instruct.MuxMA, Instruct.MuxMB,
			Instruct.SF, Instruct.WS);

	DoADD();

	DoMUL();

	append(cOpS[Instruct.Sig]);
}

/*const char *qpu_ldi_unpack(uint32_t unpack, uint32_t data)
{
	char *tmp = tmpalloc(128);
	// unpack = 1 (2 bit signed vectors), 3 = (2 bit unsigned vectors);
	if ((unpack==1) || (unpack==3)) {
		int d[16];
		for (int i=0; i<16; i++) {
			d[i] = ((data >> (16+i-1))&0x2) | ((data >> i) & 0x1);
			if ((unpack == 1) && d[i] &0x2)
				d[i] |= 0xfffffffc;
		}
		sprintf(tmp, "[%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]",
			d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
			d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
	}
	else {
		sprintf(tmp, "0x%08x", data);
	}
	return tmp;
}*/

void Disassembler::DoLDI()
{
	if (PrintFields)
		sprintf(Comment, " md%d pm%d pck%X"
		                 " Acc%d Aw%02d Mcc%d Mw%02d"
		                 " sf%d ws%d",
			Instruct.LdMode, Instruct.PM, Instruct.Pack,
			Instruct.CondA, Instruct.WAddrA, Instruct.CondM, Instruct.WAddrM,
			Instruct.SF, Instruct.WS);

	append(cOpL[Instruct.LdMode]);
	if (Instruct.LdMode >= Inst::L_SEMA)
		append(Instruct.SA() ? "acq" : "rel");
	if (Instruct.SF)
		append(".setf");
	append(" ");

	if (Instruct.WAddrA != Inst::R_NOP)
		appendf("%s%s%s, ", cWreg[Instruct.WS][Instruct.WAddrA], packCode(false), cCC[Instruct.CondA]);
	if (Instruct.WAddrM != Inst::R_NOP)
		appendf("%s%s%s, ", cWreg[!Instruct.WS][Instruct.WAddrM], packCode(true), cCC[Instruct.CondM]);
	if (Instruct.WAddrA == Inst::R_NOP && Instruct.WAddrM == Inst::R_NOP)
		append("-, ");

	switch (Instruct.LdMode)
	{default:
		return appendImmd(Instruct.Immd);
	 case Inst::L_PES:
		return appendPE(Instruct.Immd.uValue, true);
	 case Inst::L_PEU:
		return appendPE(Instruct.Immd.uValue, false);
	}
}

void Disassembler::DoBranch()
{
	if (PrintFields)
		sprintf(Comment, " rel%d reg%d ra%02d pm%d upk%d Bcc%X"
		                 " Aw%02d Mw%02d ws%d",
			Instruct.Rel, Instruct.Reg, Instruct.RAddrA, Instruct.PM, Instruct.Unpack, Instruct.CondBr,
			Instruct.WAddrA, Instruct.WAddrM, Instruct.WS);

	appendf("%s%s ", Instruct.Rel ? "brr" : "bra", cBCC[Instruct.CondBr]);
	if (Instruct.WAddrA != Inst::R_NOP)
		appendf("%s, ", cWreg[Instruct.WS][Instruct.WAddrA]);
	if (Instruct.WAddrM != Inst::R_NOP)
		appendf("%s, ", cWreg[!Instruct.WS][Instruct.WAddrM]);
	if (Instruct.WAddrA == Inst::R_NOP && Instruct.WAddrM == Inst::R_NOP)
		append("-, ");
	if (Instruct.Reg)
	{	append(cRreg[0][Instruct.RAddrA]);
		if (Instruct.Immd.iValue)
			append(", ");
	}
	// try label
	if (Instruct.Immd.iValue)
	{	uint32_t target = Instruct.Immd.uValue;
		if (Instruct.Rel)
			target += Addr + 4*sizeof(uint64_t);
		auto l = Labels.find(target);
		if (l != Labels.end())
			return appendf(Instruct.Rel ? "r:%s" : ":%s", l->second.c_str());
		return appendf(Instruct.Rel ? "%+d # %04x" : "%d # %04x", Instruct.Immd.iValue, target);
	}
	if (Instruct.Immd.iValue || !Instruct.Reg)
		appendf(Instruct.Rel ? "%+d" : "0x%x", Instruct.Immd.iValue);
}

void Disassembler::DoInstruction()
{
	CodeAt = Code;
	*Comment = 0;
	switch (Instruct.Sig)
	{case Inst::S_BRANCH:
		DoBranch(); break;
	 case Inst::S_LDI:
		DoLDI(); break;
	 default:
		DoALU(); break;
	}
}

void Disassembler::ScanLabels()
{
	Addr = BaseAddr + 3*sizeof(uint64_t);
	for (uint64_t i : Instructions)
	{	Addr += sizeof(uint64_t); // base now points to branch point.
		Instruct.decode(i);
		if (Instruct.Sig != Inst::S_BRANCH) // only branch instructions
			continue;
		// link address
		if (Instruct.WAddrA != Inst::R_NOP)
			Labels.emplace(Addr, stringf("LL%zu_%s", Labels.size(), cWreg[Instruct.WS][Instruct.WAddrA]));
		if (Instruct.WAddrM != Inst::R_NOP)
			Labels.emplace(Addr, stringf("LL%zu_%s", Labels.size(), cWreg[!Instruct.WS][Instruct.WAddrM]));
		if (!Instruct.Immd.iValue)
			continue;
		if (Instruct.Rel)
			Labels.emplace(Addr + Instruct.Immd.iValue, stringf("L%x_%x", Addr + Instruct.Immd.iValue, Addr - 4*(unsigned)sizeof(uint64_t)));
		else
			Labels.emplace(Instruct.Immd.uValue, stringf("L%x_%x", Instruct.Immd.uValue, Addr - 4*(unsigned)sizeof(uint64_t)));
	}
}

void Disassembler::Disassemble()
{
	Addr = BaseAddr;
	for (uint64_t i : Instructions)
	{	Instruct.decode(i);
		// Label?
		auto l = Labels.find(Addr);
		if (l != Labels.end())
			fprintf(Out, ":%s\n", l->second.c_str());

		DoInstruction();
		*CodeAt = 0;
		if (PrintComment)
			fprintf(Out, "\t%-48s # %04x: %0" PRIx64 " %s\n", Code, Addr, i, Comment);
		else
			fprintf(Out, "\t%s\n", Code);
		Addr += sizeof(uint64_t);
	}
}
