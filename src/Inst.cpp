/*
 * Instruction.cpp
 *
 *  Created on: 25.10.2014
 *      Author: mueller
 */

#define __STDC_FORMAT_MACROS // Work around for older g++

#include "Inst.h"
#include "utils.h"

#include <cfloat>
#include <cmath>
#include <cinttypes>


qpuValue& qpuValue::operator=(const exprValue& value)
{
	switch (value.Type)
	{default:
		throw stringf("Value of type %s cannot be used for QPU evaluation. Only constants are allowed.", type2string(value.Type));

	 case V_LDPES:
	 case V_LDPE:
	 case V_LDPEU:
	 case V_INT:
		if (value.iValue < INT32_MIN || value.iValue > UINT32_MAX)
			throw stringf("Integer constant 0x%" PRIx64 " out of range for use as QPU constant.", value.iValue);
		iValue = (int32_t)value.iValue;
		break;

	 case V_FLOAT:
		if (fabs(value.fValue) > FLT_MAX)
			throw stringf("Floating point constant %f does not fit into 32 bit float.", value.fValue);
		fValue = (float)value.fValue;
	}
	return *this;
}


static inline void adds(uint8_t& l, uint8_t r)
{	l = (uint8_t)min(255, (l+r));
}
static inline void subs(uint8_t& l, uint8_t r)
{	l = (uint8_t)max(0, (l-r));
}
static inline void muld(uint8_t& l, uint8_t r)
{	// TODO: what are the rounding rules of VideoCore IV
	l = (uint8_t)((l * r) / 65025);
}

bool Inst::evalADD(qpuValue& l, qpuValue r)
{
	switch (OpA)
	{default:
		return false;
	 case A_FADD:
		l.fValue += r.fValue; break;
	 case A_FSUB:
		l.fValue += r.fValue; break;
	 case A_FMIN:
		l.fValue = min(l.fValue, r.fValue); break;
	 case A_FMAX:
		l.fValue = max(l.fValue, r.fValue); break;
	 case A_FMINABS:
		l.fValue = min(fabsf(l.fValue), fabsf(r.fValue)); break;
	 case A_FMAXABS:
		l.fValue = max(fabsf(l.fValue), fabsf(r.fValue)); break;
	 case A_FTOI:
		l.iValue = (int)floorf(r.fValue + .5); break;
	 case A_ITOF:
		l.fValue = r.iValue; break;
	 case A_ADD:
		l.iValue += r.iValue; break;
	 case A_SUB:
		l.iValue -= r.iValue; break;
	 case A_SHR:
		l.uValue >>= r.uValue & 0x1f; break;
	 case A_ASR:
		l.iValue >>= r.iValue & 0x1f; break;
	 case A_ROR:
		r.iValue &= 0x1f;
		l.uValue = l.uValue >> r.iValue | l.uValue << (32-r.iValue); break;
	 case A_SHL:
		l.iValue <<= r.iValue & 0x1f; break;
	 case A_MIN:
		l.iValue = min(l.iValue, r.iValue); break;
	 case A_MAX:
		l.iValue = max(l.iValue, r.iValue); break;
	 case A_AND:
		l.uValue &= r.uValue; break;
	 case A_OR:
		l.uValue |= r.uValue; break;
	 case A_XOR:
		l.uValue ^= r.uValue; break;
	 case A_NOT:
		l.iValue = ~r.iValue; break;
	 case A_CLZ:
		if (r.uValue)
		{	l.fValue = r.uValue;
			l.iValue = 0x9e - (l.uValue >> 23); // get float exponent
		} else
			l.iValue = 32;
		break;
	 case A_V8ADDS:
		adds(l.cValue[0], r.cValue[0]);
		adds(l.cValue[1], r.cValue[1]);
		adds(l.cValue[2], r.cValue[2]);
		adds(l.cValue[3], r.cValue[3]);
		break;
	 case A_V8SUBS:
		subs(l.cValue[0], r.cValue[0]);
		subs(l.cValue[1], r.cValue[1]);
		subs(l.cValue[2], r.cValue[2]);
		subs(l.cValue[3], r.cValue[3]);
		break;
	}
	return true;
}
bool Inst::evalMUL(qpuValue& l, qpuValue r)
{	switch (OpM)
	{default:
		return false;
	 case M_FMUL:
		l.fValue *= r.fValue; break;
	 case M_MUL24:
		r.iValue &= 0xFFFFFF;
		l.iValue &= 0xFFFFFF;
		l.iValue *= r.iValue; break;
	 case M_V8MULD:
		muld(l.cValue[0], r.cValue[0]);
		muld(l.cValue[1], r.cValue[1]);
		muld(l.cValue[2], r.cValue[2]);
		muld(l.cValue[3], r.cValue[3]);
		break;
	 case M_V8MIN:
		l.cValue[0] = min(l.cValue[0], r.cValue[0]);
		l.cValue[1] = min(l.cValue[1], r.cValue[1]);
		l.cValue[2] = min(l.cValue[2], r.cValue[2]);
		l.cValue[3] = min(l.cValue[3], r.cValue[3]);
		break;
	 case M_V8MAX:
		l.cValue[0] = max(l.cValue[0], r.cValue[0]);
		l.cValue[1] = max(l.cValue[1], r.cValue[1]);
		l.cValue[2] = max(l.cValue[2], r.cValue[2]);
		l.cValue[3] = max(l.cValue[3], r.cValue[3]);
		break;
	 case M_V8ADDS:
		adds(l.cValue[0], r.cValue[0]);
		adds(l.cValue[1], r.cValue[1]);
		adds(l.cValue[2], r.cValue[2]);
		adds(l.cValue[3], r.cValue[3]);
		break;
	 case M_V8SUBS:
		subs(l.cValue[0], r.cValue[0]);
		subs(l.cValue[1], r.cValue[1]);
		subs(l.cValue[2], r.cValue[2]);
		subs(l.cValue[3], r.cValue[3]);
		break;
	}
	return true;
}

bool Inst::evalPack(qpuValue& r, qpuValue v, bool mul)
{	int32_t mask = -1;
	if (PM)
	{	if (mul) // MUL ALU pack mode?
			switch (Pack)
			{default:
				return false;

			 case P_8a:
			 case P_8b:
			 case P_8c:
			 case P_8d:
				mask = 0xff << ((Pack & 3) * 8);
			 case P_8abcd:
				if (v.fValue <= 0.F)
					v.iValue = 0;
				else if (v.fValue >= 1.0F)
					v.iValue = 255;
				else
					v.iValue = (int)(v.fValue * 255); // TODO: is the rounding correct?
				v.iValue *= 0x1010101; // replicate bytes
				r.iValue &= ~mask;
				r.iValue |= v.iValue;
				return true;

			 case P_32:;
			}
	} else
	{	if (mul == WS) // regfile A pack mode?
			switch (Pack)
			{case P_16a:
				mask = 0xffff;
				goto pack16_cont;
			 case P_16b:
				mask = 0xffff0000;
				goto pack16_cont;
			 case P_16aS:
				mask = 0xffff;
				goto pack16S_cont;
			 case P_16bS:
				mask = 0xffff0000;
			 pack16S_cont:
				if (v.iValue < 0x8000)
					v.iValue = 0x8000;
				else if (v.iValue > 0x7fff)
					v.iValue = 0x7fff;
			 pack16_cont:
				if (mul ? OpM == M_MUL24 : 0x17e & (1<<OpA))
					throw string("The 16 bit floating point pack modes are not yet implemented."); // TODO
				v.iValue &= 0xffff;
				v.iValue *= 0x10001; // replicate words
				goto pack_cont;

			 case P_8a:
			 case P_8b:
			 case P_8c:
			 case P_8d:
				mask = 0xff << ((Pack & 3) * 8);
				goto pack8_cont;
			 case P_8aS:
			 case P_8bS:
			 case P_8cS:
			 case P_8dS:
				mask = 0xff << ((Pack & 3) * 8);
			 case P_8abcdS:
				if (v.iValue < 0)
					v.iValue = 0;
				else if (v.iValue > 255)
					v.iValue = 255;
			 case P_8abcd:
			 pack8_cont:
				v.iValue &= 0xff;
				v.iValue *= 0x1010101; // replicate bytes
			 pack_cont:
				r.iValue &= ~mask;
				r.iValue |= v.iValue;
				return true;

			 case P_32S:
				throw string("The 32 bit pack mode with saturation is not yet implemented."); // TODO

			 case P_32:;
			}
	}
	// no pack
	r = v;
	return true;
}

void Inst::reset()
{	Sig = S_NONE;
	WS = false;
	WAddrA = R_NOP;
	WAddrM = R_NOP;
	RAddrA = R_NOP;
	Unpack = U_32;
	RAddrB = R_NOP;
	MuxAA = X_R0;
	MuxAB = X_R0;
	MuxMA = X_R0;
	MuxMB = X_R0;
	OpM = M_NOP;
	OpA = A_NOP;
	PM = false;
	Pack = P_32;
	CondA = C_AL;
	CondM = C_AL;
	SF = false;
}

bool Inst::trySwap(bool mul)
{
	if ( SF             // can't swap with set flags
		|| (PM && Pack) ) // can't swap with MUL ALU pack
		return false;
	if (mul)
	{	if (OpA != A_NOP)
			return false;
		switch (OpM)
		{default:
			return false;
		 case M_NOP:
			return true;
		 case M_V8ADDS:
			OpA = A_V8ADDS; break;
		 case M_V8SUBS:
			OpA = A_V8SUBS; break;
		 case M_V8MIN:
		 case M_V8MAX:
			if (MuxMA != MuxMB)
				return false;
			OpA = A_OR;    break;
		}                OpM = M_NOP;
		MuxAA = MuxMA;   MuxMA = X_R0;
		MuxAB = MuxMB;   MuxMB = X_R0;
	} else
	{	if (OpM != M_NOP)
			return false;
		switch (OpA)
		{default:
			return false;
		 case A_NOP:
			return true;
		 case A_V8ADDS:
			OpM = M_V8ADDS; break;
		 case A_XOR:
		 case A_SUB:
			if (MuxMA != MuxMB)
				return false;
		 case A_V8SUBS:
			OpM = M_V8SUBS; break;
		 case A_OR:
		 case A_AND:
		 case A_MIN:
		 case A_MAX:
			if (MuxMA != MuxMB)
				return false;
			OpM = M_V8MIN;  break;
		}                OpA = A_NOP;
		MuxMA = MuxAA;   MuxAA = X_R0;
		MuxMB = MuxAB;   MuxAB = X_R0;
	}
	swap(CondA, CondM);
	swap(WAddrA, WAddrM);
	WS = !WS;
	return true;
}

void Inst::optimize()
{	qpuValue val;
	switch (Sig)
	{case S_SMI:
		if (WAddrM == R_NOP && MuxAA == X_RB && MuxAB == X_RB && SImmd < 48)
		{	// convert ADD ALU small immediate loads to LDI if MUL ALU is unused.
			val = SMIValue();
			if (evalADD(val, val) && evalPack(val, val, false))
				goto mkLDI;
		}
		if (OpA == A_NOP && MuxMA == X_RB && MuxMB == X_RB && SImmd < 48)
		{	// convert ADD ALU small immediate loads to LDI if MUL ALU is unused.
			val = SMIValue();
			if (evalMUL(val, val) && evalPack(val, val, true))
				goto mkLDI;
		}
	 default:
		switch (OpA)
		{default:
			if (WAddrA != R_NOP || SF)
				break;
		 case A_NOP:
			CondA = C_NEVER;
			//MuxAA = X_R0;
			//MuxAB = X_R0;
			break;
		 case A_XOR:
		 case A_SUB:
			if (OpM == M_NOP && MuxAA == MuxAB)
				goto mkLDI0; // convert to LDI ..., 0
			break;
		}
		switch (OpM)
		{default:
			if (WAddrM != R_NOP || (SF && (OpA == A_NOP || CondA == C_NEVER)))
				break;
		 case M_NOP:
			CondM = C_NEVER;
			//MuxMA = X_R0;
			//MuxMB = X_R0;
			break;
		 case M_V8SUBS:
			if (OpA == A_NOP && MuxMA == MuxMB)
				goto mkLDI0; // convert to LDI ..., 0
			break;
		}
		break;
	 mkLDI0:
		if (Sig != S_NONE)
			break;
		val.uValue = 0;
	 mkLDI:
		Sig = S_LDI;
		LdMode = L_LDI;
		Immd = val;
		PM = false;
		Pack = P_32;
	 case S_LDI: // ldi, sema
		if (WAddrA == R_NOP && !SF)
			CondA = C_NEVER;
		if (WAddrM == R_NOP)
			CondM = C_NEVER;
	 case S_BRANCH:
		break;
	}
}

uint64_t Inst::encode() const
{	switch (Sig)
	{default:
		return (uint64_t)
				(	Sig    << 28
				| Unpack << 25
				| PM     << 24
				| Pack   << 20
				| CondA  << 17
				| CondM  << 14
				| SF     << 13
				| WS     << 12
				| WAddrA << 6
				| WAddrM << 0 ) << 32
			| (uint32_t)
				( OpM    << 29
				| OpA    << 24
				| RAddrA << 18
				| RAddrB << 12
				| MuxAA  <<  9
				| MuxAB  <<  6
				| MuxMA  <<  3
				| MuxMB  <<  0 );
	 case S_LDI: // ldi, sema
		return (uint64_t)
				( S_LDI  << 28
				| LdMode << 25
				| PM     << 24
				| Pack   << 20
				| CondA  << 17
				| CondM  << 14
				| SF     << 13
				| WS     << 12
				| WAddrA << 6
				| WAddrM << 0 ) << 32
			| Immd.uValue;
	 case S_BRANCH: // Branch
		return (uint64_t)
				( S_BRANCH << 28
				| CondBr << 20
				| Rel    << 19
				| Reg    << 18
				| RAddrA << 13
				| WS     << 12
				| WAddrA << 6
				| WAddrM << 0 ) << 32
			| Immd.uValue;
	}
}

void Inst::decode(uint64_t code)
{	Sig    = (sig)(code >> 60);
	Unpack = (unpack)(code >> 57 & 0x7);
	PM     = !!(code & 1ULL<<56);
	if (Sig == S_BRANCH)
	{	CondBr = (condb)(code >> 52 & 0xf);
		Rel    = !!(code & 1ULL<<51);
		Reg    = !!(code & 1ULL<<50);
		RAddrA = (uint8_t)(code >> 45 & 0x1f);
		Immd.uValue = (uint32_t)(code >> 0  & 0xffffffff);
	} else
	{	Pack   = (pack)(code >> 52 & 0xf);
		CondA  = (conda)(code >> 49 & 0x7);
		CondM  = (conda)(code >> 46 & 0x7);
		SF     = !!(code & 1ULL<<45);
		if (Sig == S_LDI)
		{	Immd.uValue = (uint32_t)(code >>  0 & 0xffffffff);
		} else
		{	OpM    = (opmul)(code >> 29 & 0x7);
			OpA    = (opadd)(code >> 24 & 0x1f);
			RAddrA = (uint8_t)(code >> 18 & 0x3f);
			RAddrB = (uint8_t)(code >> 12 & 0x3f);
			MuxAA  = (mux)(code >>  9 & 0x7);
			MuxAB  = (mux)(code >>  6 & 0x7);
			MuxMA  = (mux)(code >>  3 & 0x7);
			MuxMB  = (mux)(code >>  0 & 0x7);
		}
	}
	WS     = !!(code & 1ULL<<44);
	WAddrA = (uint8_t)(code >> 38 & 0x3f);
	WAddrM = (uint8_t)(code >> 32 & 0x3f);
}

qpuValue Inst::SMIValue() const
{ qpuValue ret;
	ret.uValue = 0;
	if (SImmd < 32)
		ret.iValue = (int8_t)(SImmd << 3) >> 3; // signed expand
	else if (SImmd < 48)
		ret.iValue = ((int32_t)(SImmd ^ 0x28) << 23) + 0x3b800000;
	return ret;
}

uint8_t Inst::AsSMIValue(qpuValue i)
{	if (i.uValue + 16 <= 0x1f)
		return (uint8_t)(i.uValue & 0x1f);
	if (!((i.uValue - 0x3b800000) & 0xf87fffff))
		return (uint8_t)(((i.uValue >> 23) - 0x77) ^ 0x28);
	return 0xff; // failed
}
