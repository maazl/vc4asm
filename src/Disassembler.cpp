/*
 * Disassembler.cpp
 *
 *  Created on: 12.11.2014
 *      Author: mueller
 */

#define __STDC_FORMAT_MACROS // Work around for older g++

#include "Disassembler.h"
#include "utils.h"

#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cinttypes>

#include "Disassembler.tables.cpp"


constexpr const struct DisassemblerMSG Disassembler::MSG;


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

void Disassembler::appendImmd(qpuValue value)
{	// Check whether likely a float, just a guess
	if ((Options & O_UseFloat) && abs(((value.iValue >> 23) & 0xff) ^ 0x80) <= 20)
		CodeAt += snprintf(CodeAt, Code + sizeof Code - CodeAt, "%.7e", value.fValue);
	else if (abs(value.iValue) < 256)
		appendf("%i", value.iValue);
	else
		appendf("0x%x", value.uValue);
}

void Disassembler::appendPE(bool sign)
{	uint32_t val = Instruct.Immd.uValue;
	signed char values[16];
	if (sign)
		for (int pos = 16; pos--; val <<= 1)
			values[pos] = (((int32_t)val >> 30) & -2) | ((val >> 15) & 1);
	else
		for (int pos = 16; pos--; val <<= 1)
			values[pos] = ((val >> 30) & 2) | ((val >> 15) & 1);
	*CodeAt++ = '[';
	for (int val : values)
		appendf("%i,", val);
	CodeAt[-1] = ']';
}

void Disassembler::appendPack(bool mul)
{	if (Instruct.PM ? mul : Instruct.WS == mul)
	{	append(cPack[Instruct.PM][Instruct.Pack & 15]);
		append(cPUPX[Instruct.Pack / Inst::P_INT]);
	}
}

void Disassembler::appendSource(Inst::mux mux)
{	switch (mux)
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
		append(Inst::toString(mux));
	}
	if ( (Instruct.PM && mux == Inst::X_R4) // r4 unpack
		|| (!Instruct.PM && mux == Inst::X_RA) ) // RA unpack
	{	append(cUnpack[Instruct.Unpack & 7]);
		append(cPUPX[Instruct.Unpack / Inst::U_INT]);
	}
}

void Disassembler::appendMULSource(Inst::mux mux)
{	append(", ");
	appendSource(mux);

	if ( Instruct.Sig != Inst::S_SMI || Instruct.SImmd < 48 // no rotation
		|| (mux == Inst::X_RA && Instruct.RAddrA == 32) // register not sensitive to rotation
		|| mux == Inst::X_RB )                          // small immediate value also not
		return;
	int rot = Instruct.SImmd - 48; // [0..15]
	if (rot == 0)
	{	append(">>r5");
		return;
	}
	if (rot >= 8)
		rot -= 16;
	if (mux >= Inst::X_R4)
	{	rot %= 4;
		if (rot == 0)
			return;
	}
	if (rot < 0)
		appendf("<<%i", -rot);
	else
		appendf(">>%i", rot);
}

void Disassembler::DoADD()
{
	uint8_t opa = Instruct.OpA;
	if (cOpA[opa][0] == '?')
		Msg(MSG.INVALID_ADDOP, opa);
	bool isUnary = Instruct.isUnary();
	if (isUnary && Instruct.MuxAB)
		Msg(MSG.SECOND_SRC_UNARY_OP);
	if (Instruct.WAddrA == Inst::R_NOP && Instruct.CondA != Inst::C_NEVER && (!Instruct.SF || opa == Inst::A_NOP))
		Msg(MSG.WADDRANOP_NOT_CCNEVER);
	bool isImmd = Instruct.Sig == Inst::S_SMI && Instruct.MuxAA == Inst::X_RB
		&& (isUnary || Instruct.MuxAB == Inst::X_RB);

	if ( (Options & O_UseMOV)
		&& ( (Instruct.MuxAA == Instruct.MuxAB && (0x807c2000 & (1<<opa))) // Both inputs equal and instruction that returns identity or 0
			|| isImmd ))
		opa = 32;

	append(cOpA[opa]);

	if (Instruct.isSFADD())
		append(".setf");

	if (opa == Inst::A_NOP)
	{	if (Instruct.MuxAA || Instruct.MuxAB)
			Msg(MSG.SRC_ANOP);
		if (Instruct.WAddrA == Inst::R_NOP && Instruct.CondA == Inst::C_NEVER)
			return;
	}

	append(cCC[Instruct.CondA]);
	append(" ");

	// Parameters for ADD ALU
	// Target
	append(cWreg[Instruct.WS][Instruct.WAddrA]);
	bool unpack = false;
	if ((Options & O_UseMOV) && isImmd && (0x0909 & (1<<Instruct.Pack)))
		unpack = true;
	else
		appendPack(false);

	switch (opa)
	{case Inst::A_NOP:
		return;

	 case 32:
		switch (Instruct.OpA)
		{case Inst::A_SUB:
		 case Inst::A_XOR:
		 case Inst::A_V8SUBS:
			return append(", 0");
		 default:;
		}
		if (isImmd)
		{	// constant => evaluate
			auto value(Instruct.SMIValue());
			Instruct.evalADD(value, value);
			if (unpack)
				Instruct.evalPack(value, value, false);
			append(", ");
			appendImmd(value);
			return;
		}
	}

	append(", ");
	appendSource(Instruct.MuxAA);

	if (opa != 32 && !isUnary)
	{	append(", ");
		appendSource(Instruct.MuxAB);
	}
}

void Disassembler::DoMUL()
{
	if (Instruct.WAddrM == Inst::R_NOP)
	{	if (Instruct.CondM != Inst::C_NEVER && (!Instruct.SF || Instruct.OpA != Inst::A_NOP))
			Msg(MSG.WADDRMNOP_NOT_CCNEVER);
		if (Instruct.OpM == Inst::M_NOP)
		{nop:
			if (Instruct.MuxMA || Instruct.MuxMB)
				Msg(MSG.SRC_MNOP);
			return;
	}	}

	uint8_t opm = Instruct.OpM;
	bool isSMI = Instruct.Sig == Inst::S_SMI && Instruct.MuxMA == Inst::X_RB;

	if ( (Options & O_UseMOV) && Instruct.MuxMA == Instruct.MuxMB // Both inputs equal and
		&& ( (0xb0 & (1<<opm)) // instruction that returns identity or 0 or
			|| isSMI )) // small immediate
		opm = 8;

	append(";  ");
	append(cOpM[opm]);

	if (Instruct.isSFMUL())
		append(".setf");

	append(cCC[Instruct.CondM]);

	append(" ");
	append(cWreg[!Instruct.WS][Instruct.WAddrM]);
	bool unpack = false;
	if (opm == 8 && isSMI && (0x0009 & (1<<Instruct.Pack)))
		unpack = true;
	else
		appendPack(true);

	switch (opm)
	{case Inst::M_NOP:
		goto nop;

	 default:
		appendMULSource(Instruct.MuxMA);
		break;

	 case 8:
		if (Instruct.OpM == Inst::M_V8SUBS)
			return append(", 0");
		if (isSMI)
		{	// constant => evaluate
			auto value(Instruct.SMIValue());
			Instruct.evalMUL(value, value);
			if (unpack)
				Instruct.evalPack(value, value, true);
			append(", ");
			appendImmd(value);
			return;
		}
	}
	appendMULSource(Instruct.MuxMB);
}

void Disassembler::DoRead(Inst::mux regfile)
{
	if ( Instruct.MuxAA != regfile && Instruct.MuxAB != regfile
		&& Instruct.MuxMA != regfile && Instruct.MuxMB != regfile )
	{	append(";  read ");
		appendSource(regfile);
	}
}

void Disassembler::DoALU()
{
	if (Instruct.Pack && Instruct.PM && (Instruct.Pack & 15) < Inst::P_8abcdS)
		Msg(MSG.INVALID_MUL_PACK);

	DoADD();

	DoMUL();

	if (Instruct.RAddrA != Inst::R_NOP)
		DoRead(Inst::X_RA);
	if ( Instruct.Sig == Inst::S_SMI
		? Instruct.SImmd < 48
		: Instruct.RAddrB != Inst::R_NOP )
	DoRead(Inst::X_RB);

	append(cOpS[Instruct.Sig]);
}

void Disassembler::DoLDI()
{
	if (Instruct.LdMode == 2 || Instruct.LdMode > Inst::L_SEMA)
		Msg(MSG.INVALID_LDI_MODE);

	append(cOpL[Instruct.LdMode]);
	if (Instruct.LdMode >= Inst::L_SEMA)
		append(Instruct.SA() ? "acq" : "rel");
	if (Instruct.SF)
		append(".setf");
	append(" ");

	if (Instruct.WAddrA != Inst::R_NOP)
	{	append(cWreg[Instruct.WS][Instruct.WAddrA]);
		appendPack(false);
		append(cCC[Instruct.CondA]);
		append(", ");
	}
	if (Instruct.WAddrM != Inst::R_NOP)
	{	append(cWreg[!Instruct.WS][Instruct.WAddrM]);
		appendPack(true);
		append(cCC[Instruct.CondM]);
		append(", ");
	}
	if (Instruct.WAddrA == Inst::R_NOP && Instruct.WAddrM == Inst::R_NOP)
		append("-, ");

	switch (Instruct.LdMode)
	{default:
		return appendImmd(Instruct.Immd);
	 case Inst::L_PES:
		return appendPE(true);
	 case Inst::L_PEU:
		return appendPE(false);
	}
}

void Disassembler::DoBranch()
{
	if (Instruct.Unpack || Instruct.PM)
		Msg(MSG.BRANCH_NO_UNPACK);
	if (cBCC[Instruct.CondBr][1] == '?')
		Msg(MSG.INVALID_BRANCH_CC);

	append(Instruct.Rel ? "brr" : "bra");
	append(cBCC[Instruct.CondBr]);
	if (Instruct.SF)
		append(".setf");
	append(" ");
	// Destination registers
	int have_dest = (Instruct.WAddrA != Inst::R_NOP) | ((Instruct.WAddrM != Inst::R_NOP) << 1);
	switch (have_dest)
	{case 0:
		append("-, "); break;
	 default:
		append(cWreg[Instruct.WS][Instruct.WAddrA]), append(", ");
		if (have_dest == 1)
			break;
	 case 2:
		append(cWreg[!Instruct.WS][Instruct.WAddrM]), append(", ");
	}
	// Branch target
	if (Instruct.Reg)
	{	append(cRreg[0][Instruct.RAddrA]);
		if (Instruct.Immd.iValue || have_dest == 3)
			append(", ");
	} else if (have_dest == 3)
		append("-, ");
	// try label
	if (Instruct.Immd.iValue)
	{	unsigned target = Instruct.Immd.uValue;
		if (Instruct.Rel)
			target += Addr + 3*sizeof(uint64_t);
		string l = GetLabel(target);
		if (l.length() != 0)
		{	if(Instruct.Rel)
				append("r");
			append(l.c_str());
		} else
			appendf(Instruct.Rel ? "%+d # %04x" : "%d # %04x", Instruct.Immd.iValue, target);
	} else if (!Instruct.Reg || have_dest == 3)
		appendf(Instruct.Rel ? "%+d" : "0x%x", Instruct.Immd.iValue);
}

string Disassembler::GetLabel(unsigned addr) const
{
	auto l = Labels.find(addr);
	return l == Labels.end() ? string() : ':' + l->second;
}

void Disassembler::PushInstruction(uint64_t inst)
{
	Instruct.decode(inst);
	Addr += sizeof(uint64_t);
}

void Disassembler::ScanLabels()
{
	if (Instruct.Sig != Inst::S_BRANCH) // only branch instructions
		return;
	// link address
	unsigned target = Addr + 3*sizeof(uint64_t);
	if (Instruct.WAddrA != Inst::R_NOP)
		Labels.emplace(target, stringf("LL%zu_%s", Labels.size(), cWreg[Instruct.WS][Instruct.WAddrA]));
	if (Instruct.WAddrM != Inst::R_NOP)
		Labels.emplace(target, stringf("LL%zu_%s", Labels.size(), cWreg[!Instruct.WS][Instruct.WAddrM]));
	if (!Instruct.Immd.iValue)
		return;
	if (Instruct.Rel)
		Labels.emplace(target + Instruct.Immd.iValue, stringf("L%x_%x", target + Instruct.Immd.iValue, Addr - sizeof(uint64_t)));
	else
		Labels.emplace(Instruct.Immd.uValue, stringf("L%x_%x", Instruct.Immd.uValue, Addr - sizeof(uint64_t)));
}

string Disassembler::Disassemble()
{
	CodeAt = Code;
	switch (Instruct.Sig)
	{case Inst::S_BRANCH:
		DoBranch(); break;
	 case Inst::S_LDI:
		DoLDI(); break;
	 default:
		DoALU(); break;
	}
	*CodeAt = 0;
	return Code;
}

string Disassembler::GetFields()
{
	switch (Instruct.Sig)
	{case Inst::S_BRANCH:
		return stringf(
			"rel%d reg%d ra%02d pm%d upk%d Bcc%X "
			"Aw%02d Mw%02d ws%d",
			Instruct.Rel, Instruct.Reg, Instruct.RAddrA, Instruct.PM, Instruct.Unpack, Instruct.CondBr,
			Instruct.WAddrA, Instruct.WAddrM, Instruct.WS);
	 case Inst::S_LDI:
		return stringf(
			"md%d pm%d pck%X "
			"Acc%d Aw%02d Mcc%d Mw%02d "
			"sf%d ws%d",
			Instruct.LdMode, Instruct.PM, Instruct.Pack,
			Instruct.CondA, Instruct.WAddrA, Instruct.CondM, Instruct.WAddrM,
			Instruct.SF, Instruct.WS);
	 default:
		return stringf(
			"sig%X ra%02d rb%02d pm%d upk%d pck%X "
			"Aop%02d Acc%d Aw%02d Aa%d Ab%d "
			"Mop%d Mcc%d Mw%02d Ma%d Mb%d "
			"sf%d ws%d",
			Instruct.Sig, Instruct.RAddrA, Instruct.RAddrB, Instruct.PM, Instruct.Unpack, Instruct.Pack,
			Instruct.OpA, Instruct.CondA, Instruct.WAddrA, Instruct.MuxAA, Instruct.MuxAB,
			Instruct.OpM, Instruct.CondM, Instruct.WAddrM, Instruct.MuxMA, Instruct.MuxMB,
			Instruct.SF, Instruct.WS);
	}
}
