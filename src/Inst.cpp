/*
 * Instruction.cpp
 *
 *  Created on: 25.10.2014
 *      Author: mueller
 */

#include "Inst.h"
#include <math.h>

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

bool Inst::eval(opadd op, value_t& l, value_t r)
{	switch (op)
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
		l.uValue >>= r.uValue; break;
	 case A_ASR:
		l.iValue >>= r.iValue; break;
	 case A_ROR:
		l.uValue = l.uValue >> r.iValue | l.uValue << (32-r.iValue); break;
	 case A_SHL:
		l.iValue <<= r.iValue; break;
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
bool Inst::eval(opmul op, value_t& l, value_t r)
{	switch (op)
	{default:
		return false;
	 case M_FMUL:
		l.fValue *= r.fValue; break;
	 case M_MUL24:
		// TODO: 24 bit mask
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

void Inst::optimize()
{	switch (Sig)
	{case S_SMI:
		if (OpM == M_NOP && MuxAA == X_RB && MuxAB == X_RB && SImmd < 48)
		{	// convert ADD ALU small immediate loads to LDI if MUL ALU is unused.
			value_t val = SMIValue();
			if (eval(OpA, val, val))
			{	Sig = S_LDI;
				LdMode = L_LDI;
				Immd = val;
				goto ldi;
			}
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
			{	// convert to LDI ..., 0
				Sig = S_LDI;
				LdMode = L_LDI;
				Immd.uValue = 0;
			}
			break;
		}
		if (OpM == M_NOP || WAddrM == R_NOP)
		{	CondM = C_NEVER;
			//MuxMA = X_R0;
			//MuxMB = X_R0;
		}
		break;
	 case S_LDI: // ldi, sema
	 ldi:
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

bool Inst::isVirgin() const
{	return Sig == S_NONE
		&& WAddrA == R_NOP && WAddrM == R_NOP
		&& RAddrA == R_NOP && RAddrB == R_NOP
		&& OpA == A_NOP && OpM == M_NOP
		&& !SF;
}

value_t Inst::SMIValue() const
{ value_t ret;
	ret.uValue = 0;
	if (SImmd < 32)
		ret.iValue = (int8_t)SImmd << 5 >> 5; // signed expand
	else if (SImmd < 48)
		ret.iValue = ((int32_t)(SImmd ^ 40) << 23) + 0x3b800000;
	return ret;
}
