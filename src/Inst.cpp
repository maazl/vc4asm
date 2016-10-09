/*
 * Instruction.cpp
 *
 *  Created on: 25.10.2014
 *      Author: mueller
 */

#define __STDC_FORMAT_MACROS // Work around for older g++

#include "Inst.h"
#include "Eval.h"
#include "utils.h"

#include <cfloat>
#include <cmath>
#include <cinttypes>


static inline void adds(uint8_t& l, uint8_t r)
{	l = (uint8_t)min(255, (l+r));
}
static inline void subs(uint8_t& l, uint8_t r)
{	l = (uint8_t)max(0, (l-r));
}
static inline void muld(uint8_t& l, uint8_t r)
{	l = (uint8_t)((l*r + 127) / 255);
}

bool Inst::evalADD(qpuValue& l, qpuValue r)
{
	switch (OpA)
	{default:
		return false;
	 case A_FADD:
		if (!isfinite(r.fValue)) // Emulate broken inf/nan support
			l.uValue = (r.uValue & 0x80000000) | 0x7f800000;
		else if (!isfinite(l.fValue))
			l.uValue = (l.uValue & 0x80000000) | 0x7f800000;
		else
			l.fValue += r.fValue;
		break;
	 case A_FSUB:
		if (!isfinite(r.fValue)) // Emulate broken inf/nan support
			l.uValue = (~r.uValue & 0x80000000) | 0x7f800000;
		else if (!isfinite(l.fValue))
			l.uValue = (l.uValue & 0x80000000) | 0x7f800000;
		else
		l.fValue += r.fValue;
		break;
	 case A_FMINABS:
		l.uValue &= 0x7fffffff;
		r.uValue &= 0x7fffffff;
	 case A_FMIN:
		l.fValue = l.fValue > r.fValue ? r.fValue : l.fValue;
		break;
	 case A_FMAXABS:
		l.uValue &= 0x7fffffff;
		r.uValue &= 0x7fffffff;
	 case A_FMAX:
		l.fValue = l.fValue > r.fValue ? l.fValue : r.fValue;
		break;
	 case A_FTOI:
		l.iValue = !isnormal(l.fValue) ? 0 : (int)floorf(l.fValue + .5);
		break;
	 case A_ITOF:
		l.fValue = l.iValue; break;
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
		l.uValue = l.uValue >> r.iValue | l.uValue << (32-r.iValue);
		break;
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
		l.iValue = ~l.iValue; break;
	 case A_CLZ:
		if (r.uValue)
		{	l.fValue = l.uValue;
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
		if (!(r.uValue & 0x7f800000) || !(l.uValue & 0x7f800000)) // Emulate broken inf/nan support
			l.fValue = 0.;
		else if (!isfinite(r.fValue) || !isfinite(l.fValue))
			l.uValue = ((l.uValue ^ r.uValue) & 0x80000000) | 0x7f800000;
		else
			l.fValue *= r.fValue;
		break;
	 case M_MUL24:
		r.iValue &= 0xFFFFFF;
		l.iValue &= 0xFFFFFF;
		l.iValue *= r.iValue;
		break;
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
			switch (Pack & 0x7)
			{default:
				return false;

			 case P_8a:
			 case P_8b:
			 case P_8c:
			 case P_8d:
				mask = 0xff << ((Pack & 3) * 8);
			 case P_8abcd:
				if (v.fValue >= 1.0F)
					v.iValue = 255;
				else if (v.fValue > 0.F)
					v.iValue = (int)(v.fValue * 255); // TODO: is the rounding correct?
				else
					v.iValue = 0;
				v.iValue *= 0x1010101; // replicate bytes
				r.iValue &= ~mask;
				r.iValue |= v.iValue;
				return true;

			 case P_32:;
			}
	} else
	{	if (mul == WS) // regfile A pack mode?
			switch (Pack & 0xf)
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
				if (isFloatResult(mul))
					v.uValue = toFloat16(v.fValue);
				else
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

uint64_t Inst::encode() const
{	switch (Sig)
	{default:
		return (uint64_t)
				(	Sig    << 28
				| (Unpack & 7) << 25
				| PM     << 24
				| (Pack & (15 >> PM)) << 20
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
				| (Pack & (15 >> PM)) << 20
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
				| (RAddrA|SF) << 13
				| WS     << 12
				| WAddrA << 6
				| WAddrM << 0 ) << 32
			| Immd.uValue;
	}
}

void Inst::decode(uint64_t code)
{	uint32_t i = (uint32_t)(code >> 32);
	Sig    = (sig)(i >> 28);
	PM     = !!(i & 1U<<24);
	SF     = !!(i & 1U<<13);
	WS     = !!(i & 1U<<12);
	WAddrA = (uint8_t)(i >> 6 & 0x3f);
	WAddrM = (uint8_t)(i >> 0 & 0x3f);
	if (Sig == S_BRANCH)
	{	Unpack = (unpack)(i >> 25 & 0x7);
		CondBr = (condb)(i >> 20 & 0xf);
		Rel    = !!(i & 1U<<19);
		Reg    = !!(i & 1U<<18);
		RAddrA = (uint8_t)(i >> 13 & 0x1f);
		Immd.uValue = (uint32_t)(code);
	} else
	{	Pack   = (pack)(i >> 20 & 0xf);
		CondA  = (conda)(i >> 17 & 0x7);
		CondM  = (conda)(i >> 14 & 0x7);
		if (Sig == S_LDI)
		{	Immd.uValue = (uint32_t)(code);
			LdMode = (ldmode)(i >> 25 & 7);
			if (Pack && !PM)
				(uint8_t&)Pack |= P_INT;
		} else
		{	Unpack = (unpack)(i >> 25 & 0x7);
			i = (uint32_t)code;
			OpM    = (opmul)(i >> 29 & 0x7);
			OpA    = (opadd)(i >> 24 & 0x1f);
			RAddrA = (uint8_t)(i >> 18 & 0x3f);
			RAddrB = (uint8_t)(i >> 12 & 0x3f);
			MuxAA  = (mux)(i >> 9 & 0x7);
			MuxAB  = (mux)(i >> 6 & 0x7);
			MuxMA  = (mux)(i >> 3 & 0x7);
			MuxMB  = (mux)(i >> 0 & 0x7);
			if (Pack && !PM)
				(uint8_t&)Pack |= isFloatResult(WS) ? P_FLT : P_INT;
		}
		if (PM && (Pack ^ 8) >= 0xb)
			(uint8_t&)Pack |= P_32S|P_FLT;
	}
	if (Sig != S_LDI && Unpack && Unpack != U_8dr)
		(uint8_t&)Unpack |= PM || isFloatRA() ? U_FLT : U_INT;
}

qpuValue Inst::SMIValue() const
{ qpuValue ret;
	ret.uValue = 0;
	if (SImmd < 32 || SImmd >= 48)
		ret.iValue = (int8_t)(SImmd << 3) >> 3; // signed expand
	else
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

static const char cAcc[6][4] =
{	"r0", "r1", "r2", "r3", "r4", "r5"
};
static const char cRF[2][10] =
{	"regfile A", "regfile B"
};
const char* Inst::toString(mux m)
{	return m >= X_RA ? cRF[m-X_RA] : cAcc[m];
}

unsigned Inst::toFloat16(float value)
{	// vc4asm supports only IEEE 754 floats, so we can just use bit arithmetics.
	int32_t iValue = *(int32_t*)&value;
	unsigned ret = (iValue >> 16) & 0x8000; // copy sign
	value = fabs(value); // clear sign
	if (value < 1./16384) // subnormal values and 0.
		return ret; // QPU cannot deal with subnormals  | (int)(value * 16777216.);
	if (value > 65504. && !std::isinf(value))
		return ret | 0x7c00;
	// Move exponent as well as mantissa into target including INF and NAN.
	return ret | ((iValue >> 13) & 0x7fff);
}

float Inst::fromFloat16(unsigned value)
{	// vc4asm supports only IEEE 754 floats, so we can just use bit arithmetics.
	int32_t ret = ((value & 0x8000) << 16) // copy sign
		| (((int)value << 17 >> 4) & 0x7fffffff); // expand sign of exponent
	return *(float*)&ret;
}
