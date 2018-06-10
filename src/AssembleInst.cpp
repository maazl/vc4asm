/*
 * AssembleInst.cpp
 *
 * Created on: 19.06.2016
 *   Author: mueller
 */

#define __STDC_FORMAT_MACROS // Work around for older g++

#include "AssembleInst.h"

#include <cstdio>
#include <cstdlib>
#include <cinttypes>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <inttypes.h>

#include "AssembleInst.tables.cpp"

#ifndef UINT64_MAX
#define UINT64_MAX (~(uint64_t)0)
#endif


constexpr const struct AssembleInstMSG AssembleInst::MSG;


void AssembleInst::ThrowMessage(Message&& msg) const
{	throw move(msg);
}


qpuValue AssembleInst::QPUValue(const exprValue& value)
{	qpuValue ret;
	if (value.Type == V_FLOAT)
	{	if (fabs(value.fValue) > FLT_MAX && !std::isinf(value.fValue))
		{	Msg(MSG.FLOAT_OUT_OF_RANGE, value.fValue);
			ret.fValue = value.fValue > 0 ? INFINITY : -INFINITY;
		} else
			ret.fValue = (float)value.fValue;
	} else
	{	assert(value.Type <= V_LDPEU && value.Type >= V_INT); // Type cannot be used for QPU evaluation.
		if (value.iValue < -0x80000000LL || value.iValue > 0xffffffffU)
			Fail(MSG.INT_OUT_OF_RANGE, value.iValue);
		ret.iValue = (int32_t)value.iValue;
	}
	return ret;
}

void AssembleInst::setMux(mux val)
{
	switch (+InstCtx & (IC_MUL|IC_SRC))
	{case IC_SRCA:
		MuxAA = val; break;
	 case IC_SRC:
		MuxAA = val;
	 case IC_SRCB:
		MuxAB = val; break;
	 case IC_MUL|IC_SRCA:
		MuxMA = val; break;
	 case IC_MUL|IC_SRC:
		MuxMA = val;
	 case IC_MUL|IC_SRCB:
		MuxMB = val;
	 default:;
	}
}

Inst::mux AssembleInst::muxReg(reg_t reg)
{
	if (reg.Type & R_SEMA)
		Fail(MSG.NO_SEMA_SOURCE);
	if (!(reg.Type & R_READ))
	{	// direct read access for r0..r5.
		if ((reg.Num ^ 32U) <= 5U)
			return (mux)(reg.Num ^ 32);
		Fail(MSG.SRC_REGISTER_NO_READ);
	}
	// try RA
	if (reg.Type & R_A)
	{	if ( RAddrA == reg.Num
			|| ( MuxAA != X_RA && MuxAB != X_RA
				&& MuxMA != X_RA && MuxMB != X_RA ))
		{RA:
			if (!(reg.Type & R_B))
				Flags |= IF_NORSWAP;
			RAddrA = reg.Num;
			return X_RA;
		}
	}
	// try RB
	if (reg.Type & R_B)
	{	if (Sig >= S_SMI)
			Fail(MSG.NO_REGB_VS_SMI);
		if ( RAddrB == reg.Num
			|| ( MuxAA != X_RB && MuxAB != X_RB
				&& MuxMA != X_RB && MuxMB != X_RB ))
		{RB:
			if (!(reg.Type & R_A))
				Flags |= IF_NORSWAP;
			RAddrB = reg.Num;
			return X_RB;
		}
	}
	// try to swap RA and RB of existing instruction
	switch (reg.Type & R_AB)
	{case R_A:
		if (( RAddrB == reg.Num
				|| ( MuxAA != X_RB && MuxAB != X_RB
					&& MuxMA != X_RB && MuxMB != X_RB ))
			&& !(Flags & IF_NORSWAP) && tryRABSwap() )
			goto RA;
		break;
	 case R_B:
		if (( RAddrA == reg.Num
				|| ( MuxAA != X_RA && MuxAB != X_RA
					&& MuxMA != X_RA && MuxMB != X_RA ))
			&& !(Flags & IF_NORSWAP) && tryRABSwap() )
			goto RB;
	}
	Fail(MSG.NO_REG_VS_REG);
}

const AssembleInst::smiEntry* AssembleInst::getSmallImmediateALU(uint32_t i)
{	size_t l = 0;
	size_t r = sizeof smiMap / sizeof *smiMap - 1;
	while (l < r)
	{	size_t m = (l+r) >> 1;
		if (i <= smiMap[m].Value)
			r = m;
		else
			l = m + 1;
	}
	return smiMap + r;
}

void AssembleInst::doSMI(uint8_t si)
{	switch (Sig)
	{default:
		Fail(MSG.NO_SMI_VS_SIGNAL);
	 case S_SMI:
		if (SImmd == si)
			return; // value hit
		if ((si & 16) && ((SImmd ^ si) & 31) == 0)
		{	// Vector rotation codes return the same value than [16..31]
			// => only ensure the rotation bit
			SImmd |= si;
			return;
		}
		Fail(MSG.NO_SMI_VS_SMI, si, SImmd);
	 case S_NONE:
		if (RAddrB != R_NOP)
			Fail(MSG.NO_SMI_VS_REGB);
	}
	Sig = S_SMI;
	SImmd = si;
}

void AssembleInst::applyRot(int count)
{
	if (count)
	{	if (UseRot & (IC_OP | (InstCtx & IC_SRC)))
			Fail(MSG.NO_2ND_ROT);

		if ((InstCtx & IC_BOTH) != IC_MUL && !tryALUSwap())
			Fail(MSG.ROT_REQIURES_MUL_ALU);
		if (count == 16)
			Fail(MSG.NO_ROTR_BY_R5);

		Flags |= IF_NOASWAP;
		bool isB = (InstCtx & IC_SRCB) != 0;
		auto mux = isB ? MuxMB : MuxMA;
		if (mux == X_R5)
			Msg(MSG.NO_ROT_OF_R5);

		count = 48 + (count & 0xf);
		auto isaccu = isAccu(mux);
		if (!isaccu && count > 48+3 && count < 64-3)
			Msg(MSG.NO_FULL_ROT, toString(mux));

		if (Sig == S_SMI)
		{	int mask = !isaccu
				|| ( isB && !isAccu(MuxMA)
					&& MuxAA != X_RB && MuxAB != X_RB
					&& MuxMA != X_RB && MuxMB != X_RB )
				? 0x13 : 0x1f;
			if ((SImmd ^ count) & mask)
			{	// Try to change the small immediate value without a semantic change to the other ALU.
				if (MuxAA == X_RB && MuxAB == X_RB && !SF && MuxMA != X_RB && MuxMB != X_RB)
				{	auto val = SMIValue();
					if (evalADD(val, val) && evalPack(val, val, false))
					{	for (auto si = getSmallImmediateALU(val.uValue); si->Value == val.uValue; ++si)
						{	if (si->OpCode.isMul() || ((si->SImmd ^ count) & mask))
								continue;
							// got it! replace operator and value and continue with required value
							OpA = si->OpCode.asAdd();
							SImmd = si->SImmd;
							goto gotit;
				}	}	}
				Fail(SImmd < 48 ? MSG.NO_ROT_VS_SMI : MSG.NO_ROT_VS_ROT);
			}
		 gotit:
			if (!isaccu)
				count = SImmd | 0x20;
			else
				SImmd = count;
		}

		if ( (InstCtx & IC_SRC) == IC_SRCB
			&& (Sig != S_SMI || SImmd < 48)
			&& (isAccu(MuxMA) || (count & 0x3) || !count) )
			Msg(MSG.ROT_SRC2_APPLIES_TO_SRC1);

		UseRot |= InstCtx;
		doSMI(count);

	} else if ( (InstCtx & IC_SRCB) && (UseRot & IC_SRCA)
			&& (isAccu(MuxMB) || (SImmd & 0x3) || SImmd == 48) )
		Msg(MSG.ROT_SRC1_APPLIES_TO_SRC2);
}

void AssembleInst::applyPackUnpack(rPUp mode)
{	switch (mode.requestType())
	{case rPUp::UNPACK:
		if (InstCtx & IC_DST)
			Fail(MSG.NO_UNPACK_TARGET);
	 unpack:
		doUnpack(mode.asUnPack());
		break;
	 case rPUp::PACK:
		if (InstCtx & IC_SRC)
			Fail(MSG.NO_PACK_SOURCE);
	 pack:
		doPack(mode.asPack());
		break;
	 default: // NONE
		// auto detect pack vs. unpack
		if (InstCtx & IC_SRC)
		{	if (isALU())
				goto unpack;
		} else if (InstCtx & IC_DST)
		{	if (Sig != S_BRANCH)
				goto pack;
		}
	}
}

int AssembleInst::calcPM(pack mode)
{	// valid modes: I = int source, F = float source, A = regfile A pack, M = MUL ALU pack
	// 32 bit : IA, IM, FA, FM
	// 16 bit : IA,     FA
	// 8 bit  : IA,         FM
	// 8 bitR : IA,         FM  (replicate bytes)
	uint8_t raw = mode & 15;
	if (raw == 0)
		return -1;
	if ((mode & P_INT) || raw == 8 || (raw & 7) < 3)
		return false; // regfile A pack only
	if (mode & P_FLT)
		return true;  // MUL ALU pack only
	return -1;
}

int AssembleInst::calcPM(unpack mode)
{	// valid modes: I = int result, F = float result, A = regfile A unpack, 4 = r4 unpack
	// 32 bit : IA, I4, FA, F4
	// 16 bit : IA,     FA, F4
	// 8 bit  : IA,     FA, F4
	// 8 bitR : IA, I4         (replicate alpha byte)
	int raw = mode & 7;
	if (raw == 0 || raw == 3)
		return -1;
	if (mode & U_INT)
		return false; // regfile A only
	return -1;
}

int AssembleInst::isUnpackable(mux mux, unpack mode) const
{	assert(mode);
	if (mux == X_R4)
		return true;
	if (mux == X_RA && RAddrA < 32)
		return ( (mode & (U_INT|U_FLT)) && (mode & 7) != U_8dr // Check 4 int/float
			&& (isADDFloatInput() | isMULFloatInput()) != !(mode & U_INT) ) ? -5 : false;
	return -1;
}

void AssembleInst::applyPM(bool pm)
{	if (!(Flags & IF_PMFIXED))
	{	PM = pm;
		Flags |= IF_PMFIXED;
	} else if (PM != pm)
	{	if (Pack)
			Fail(MSG.NO_UNPACK_VS_PACK);
		if (Unpack)
			if (UseUnpack & ~InstCtx & (IC_SRC|IC_BOTH))
				Fail(MSG.NO_UNPACK_VS_UNPACK);
			else
				Fail(MSG.NO_PACK_VS_UNPACK);
	}
}

void AssembleInst::doUnpack(unpack mode)
{	int pm;
	if (!mode)
	{	// no unpack request at source context...
		if (!(UseUnpack & IC_OP))
		{	// no unpack at current context...
			// ensure that no unpack silently applies.
			if ((UseUnpack & (~InstCtx & IC_BOTH)) && PM == isUnpackable(currentMux(), Unpack))
				Fail(MSG.UNPACK_APPLIES_FORM_OTHER_ALU);
			if ((InstCtx & IC_SRCB) && (UseUnpack & IC_SRCA) && PM == isUnpackable(currentMux(), Unpack))
				Msg(MSG.UNPACK_APPLIES_TO_SRC2);
		}
		return;
	}

	if (!isALU())
		Fail(MSG.NO_UNPACK_VS_BRANCH_LDI);
	if (UseUnpack & (InstCtx|IC_OP) & (IC_OP|IC_SRC))
		Fail(MSG.NO_2ND_UNPACK);
	// determine PM bit
	pm = calcPM(mode);
	if ((InstCtx & IC_SRC) && (InstCtx & IC_BOTH))
	{	int pm2 = isUnpackable(currentMux(), mode);
		if (pm2 < 0)
		{	if (pm2 == -1)
				Fail(MSG.NO_UNPACK_SRC);
			// only int/float mismatch
			if (mode & U_INT)
				Fail(MSG.NO_UNPACK_TO_INT);
			// else mode & U_FLT
			// if this is a mov instruction executed by the ADD ALU then we could fulfill the request by changing the opcode
			if ( (InstCtx & IC_SRC) == IC_SRC
				&& ((InstCtx & IC_ADD) || tryALUSwap())
				&& OpA == A_OR )
				OpA = A_FMIN;
			else
				Fail(MSG.NO_UNPACK_TO_FLOAT);
		} else if ((!pm2) == pm)
			Fail(MSG.NO_UNPACK_MODE_SRC);
		pm = pm2;
	}
	// apply unpack mode to *this
	if (pm >= 0)
		applyPM((bool)pm);
	else if (!Pack && !Unpack)
		Msg(MSG.INDETERMINATE_UNPACK);

	if (Unpack != U_32 && ((Unpack ^ mode) & 7))
		Fail(MSG.NO_UNPACK_VS_UNPACK);
	Unpack = mode;
	UseUnpack |= InstCtx;
	// Check whether unpack applies unexpectedly to another argument.
	if ((InstCtx & IC_SRCB) && !(UseUnpack & (IC_OP|IC_SRCA)) && PM == isUnpackable(getMux(InstCtx ^ IC_SRC), mode))
		Msg(MSG.UNPACK_APPLIES_TO_SRC1);
}

void AssembleInst::checkUnpack()
{
	// Check for of op-code level unpack
	if (UseUnpack & IC_OP)
	{	int pm1, pm2 = calcPM(Unpack);
		if (InstCtx & IC_MUL)
		{	pm1 = joinPM(pm2, isUnpackable(MuxMA, Unpack));
			pm2 = joinPM(pm2, isUnpackable(MuxMB, Unpack));
		} else
		{	pm1 = joinPM(pm2, isUnpackable(MuxAA, Unpack));
			if (!isUnary())
				pm2 = joinPM(pm2, isUnpackable(MuxAB, Unpack));
		}
		// pm1 := pack mode 1st src arg, pm2 := pack mode of 2nd src; <= 0 := no unpack
		if (pm1 < 0 && pm2 < 0)
			Msg(MSG.UNUSED_UNPACK);
		else if ((pm1 ^ pm2) == 1)
			Fail(MSG.AMBIGUOUS_UNPACK);
		else
			applyPM((pm1 & pm2) != 0);
	}

	// Check whether the other instruction is affected by Unpack.
	switch (InstCtx & IC_BOTH)
	{case IC_ADD:
		if (!isMUL())
			break; // MUL ALU unused => no conflict
		if ( (UseUnpack & IC_BOTH) == IC_ADD
			&& isMUL() && (isUnpackable(MuxMB, Unpack) == PM || isUnpackable(MuxMA, Unpack) == PM) )
			Fail(MSG.UNPACK_APPLIES_TO_OTHER_ALU);
		if ((UseUnpack & IC_MUL) && !PM && isADDFloatInput() && !isMULFloatInput() && (Unpack & 7) != U_8dr)
			Fail(MSG.FLOAT_AFFECTS_OTHER_ALU_UNPACK);
		break;
	 case IC_MUL:
		if (!isADD())
			break; // ADD ALU unused => no conflict
		if ( (UseUnpack & IC_BOTH) == IC_MUL
			&& (isUnpackable(MuxAA, Unpack) == PM || (!isUnary() && isUnpackable(MuxAB, Unpack) == PM)) )
			Fail(MSG.UNPACK_APPLIES_TO_OTHER_ALU);
		if ((UseUnpack & IC_ADD) && !PM && !isADDFloatInput() && isMULFloatInput() && (Unpack & 7) != U_8dr)
			Fail(MSG.FLOAT_AFFECTS_OTHER_ALU_UNPACK);
	}
}

void AssembleInst::doPack(pack mode)
{
	if (!mode)
	{	if (!(InstCtx & IC_DST) || !(InstCtx & UsePack & IC_BOTH))
			return;
		mode = Pack;
	} else
	{	if (Sig == S_BRANCH)
			Fail(MSG.NO_PACK_BRANCH_LDI);
		if (Pack)
			Fail(MSG.NO_2ND_PACK);
	}
	// handle pack mode
	{	int pm = calcPM(mode);
		switch (pm)
		{default:
			if (!(InstCtx & IC_DST))
				goto applyPack;
			// check for target
			if (InstCtx & IC_MUL)
			{	if (Unpack)
					goto applyPack;
				pm = !WS || WAddrM >= 32;
			} else
			{	pm = WS || WAddrA >= 32;
				if (!pm && Unpack)
					goto applyPack;
				if (pm && !tryALUSwap())
					Fail(MSG.ADD_ALU_PACK_NEEDS_REGA);
			}
			break; // apply PM
		 case true: // MUL ALU
			if (!(InstCtx & IC_MUL) && !tryALUSwap())
				Fail(MSG.PACK_MODE_REQUIRES_MUL_ALU);
		 case false:; // reg A
		}
		applyPM((bool)pm);
	}
 applyPack:
	if ((InstCtx & IC_DST) && !PM)
	{	if (InstCtx & IC_MUL ? (!WS || WAddrM >= 32) : (WS || WAddrA >= 32))
			Fail(MSG.PACK_MODE_REQUIRES_REGA);
		// check for int/float
		if ( (mode & (P_INT|P_FLT))
			&& (unsigned)(mode & (P_16a|P_16b|P_8a)) - P_16a <= P_16b - P_16a ) // 16 bit pack mode
		{	bool isfloat = isFloatResult(InstCtx & IC_MUL);
			if (!(mode & P_FLT) == isfloat)
			{	if (isfloat)
					Fail(MSG.PACK_MODE_REQUIRES_INT);
				/* TODO: in case of mov instruction and ADD ALU change opcode to fmin.
				else if ( (InstCtx & IC_SRC) == IC_SRC // hack for mov instruction
					&& ((InstCtx & IC_ADD) || (tryALUSwap() && OpA == A_OR)) )
					OpA = A_FMIN;*/
				else
					Fail(MSG.PACK_MODE_REQUIRES_FLOAT);
			}
		}
	}
	Pack = mode;
}

void AssembleInst::atEndOP()
{	// Actions after each (ALU) instruction
	if (isALU())
		checkUnpack();
}

/*void AssembleInst::atEndInst()
{	// Actions after each QPU instruction
}*/

bool AssembleInst::trySmallImmd(uint32_t value)
{	const smiEntry* si;
	const bool wantmul = !!(InstCtx & IC_MUL);
	if (!value)
	{	// mov ... ,0 does not require small immediate value
		si = smiMap + wantmul;
	} else
	{	switch (Sig)
		{default:
			Fail(MSG.NO_IMMD_VS_SIGNAL);
		 case S_NONE:
			if (RAddrB != R_NOP)
				Fail(MSG.NO_IMMD_VS_REGB);
		 case S_SMI:;
		}

		const smiEntry* otherALU = NULL;
		for (si = getSmallImmediateALU(value); ; ++si)
		{	if (si->Value != value)
			{	// option ALU swap?
				if (!otherALU || !tryALUSwap())
					return false; // nope, can't help
				si = otherALU;
				break; // Match!
			}
			// conflicting signal or small immediate value
			if (Sig != S_NONE && !(Sig == S_SMI && SImmd == si->SImmd))
				continue;
			// Check pack mode
			if (!!si->Pack)
			{	// conflicting pack mode?
				if (Pack != P_32 && !(((Pack ^ si->Pack) & 0xf) == 0 && PM == !(si->Pack & P_FLT)))
					continue;
				// Regfile A target
				if (!(si->Pack & P_FLT))
				{	// .setf may return unexpected results in pack mode in sign bit. TODO: accept all cases where this does not happen.
					if (SF)
						continue;
					// Not regfile A target?
					if (InstCtx & IC_MUL)
					{	if (!WS || WAddrM >= 32)
							continue;
					} else
					{	if (WS || WAddrA >= 32)
							continue;
					}
				}
			}
			// other ALU needed?
			if (si->OpCode.isMul() == wantmul)
				break; // no => Match!
			if (!otherALU)
				otherALU = si; // First option with other ALU, try later
		}

		// Match!
		Sig = S_SMI;
		SImmd = si->SImmd;
		if (!!si->Pack)
		{	Pack = si->Pack;
			PM = (si->Pack & P_FLT) != 0;
			Flags |= IF_PMFIXED;
		}
	}
	// match or mov ..., 0
	if (si->OpCode.isMul())
	{	MuxMA = MuxMB = X_RB;
		OpM  = si->OpCode.asMul();
	} else
	{	MuxAA = MuxAB = X_RB;
		OpA  = si->OpCode.asAdd();
	}
	return true;
}

void AssembleInst::applyIf(conda cond)
{
	if (Sig == S_BRANCH)
		Fail(MSG.NO_IFCC_VS_BRANCH);
	auto& target = InstCtx & IC_MUL ? CondM : CondA;
	if (target != C_AL)
		Fail(MSG.NO_2ND_IFCC);
	target = cond;
}

void AssembleInst::applySetF()
{
	if ( Sig < S_LDI && (InstCtx & IC_MUL)
		&& (WAddrA != R_NOP || OpA != A_NOP)
		&& !tryALUSwap() )
		Fail(MSG.NO_MUL_SETF_VS_ADD);
	if (SF)
		Fail(MSG.NO_2ND_SETF);
	SF = true;
	Flags |= IF_NOASWAP;
}

void AssembleInst::applyCond(condb cond)
{
	if (Sig != S_BRANCH)
		Fail(MSG.BCC_REQUIRES_BRANCH);
	if (CondBr != B_AL)
		Fail(MSG.NO_2ND_BCC);
	CondBr = cond;
}

int AssembleInst::applyADD(opadd add_op)
{	if (!isALU())
		Fail(MSG.NO_ADD_VS_BRANCH_LDI);
	if (isADD())
	{	if (isMUL())
		{fail:
			Fail(MSG.ADD_ALU_IN_USE);
		}
		switch (add_op)
		{case A_NOP:
			return applyMUL(M_NOP);
		 case A_V8ADDS:
			return applyMUL(M_V8ADDS);
		 case A_V8SUBS:
			return applyMUL(M_V8SUBS);
		 default:
			if (!tryALUSwap())
				goto fail;
		}
	}
	(uint8_t&)add_op &= 0x1f;
	OpA = add_op;
	InstCtx = IC_OP|IC_ADD;
	atInitOP();

	return add_op == A_NOP ? 0 : 1 + !isUnary();
}

int AssembleInst::applyMUL(opmul mul_op)
{	if (!isALU())
		Fail(MSG.NO_MUL_VS_BRANCH_LDI);
	if (isMUL())
	{	if (isADD())
		{fail:
			Fail(MSG.MUL_ALU_IN_USE);
		}
		switch (mul_op)
		{case M_NOP:
			return applyADD(A_NOP);
		 case M_V8ADDS:
			return applyADD(A_V8ADDS);
		 case M_V8SUBS:
			return applyADD(A_V8SUBS);
		 default:
			if (!tryALUSwap())
				goto fail;
		}
	}
	(uint8_t&)mul_op &= 7;
	OpM = mul_op;
	InstCtx = IC_OP|IC_MUL;
	atInitOP();

	return 2 * (mul_op != M_NOP);
}

void AssembleInst::applyTarget(exprValue val)
{
	if (val.Type != V_REG)
		Fail(MSG.TARGET_NEEDS_REG, val.toString().c_str());
	if (!(val.rValue.Type & R_WRITE))
		Fail(MSG.TARGET_REGISTER_NO_WRITE);

	bool mul = (InstCtx & IC_MUL) != 0;
	if ((val.rValue.Type & R_AB) != R_AB)
	{	bool wsfreeze = !isWRegAB(mul ? WAddrA : WAddrM); // Can't swap the other target register.
		if ((val.rValue.Type & R_A) && (!wsfreeze || WS == mul))
			WS = mul;
		else if ((val.rValue.Type & R_B) && (!wsfreeze || WS != mul))
			WS = !mul;
		else
			Fail(MSG.NO_SAME_REGFILE_WRITE);
	}
	(mul ? WAddrM : WAddrA) = val.rValue.Num;
	// Check pack mode from /this/ opcode if any
	if ( Sig != S_BRANCH && Pack != P_32
		&& PM == mul && !(!mul && WS) ) // Pack matches my ALU and not RA pack of MUL ALU.
	{	if (!mul)
		{	if (!WS && WAddrA < 32)
				goto packOK;
			Fail(MSG.ADD_ALU_PACK_NEEDS_REGA);
		}
		// MUL ALU => prefer Regfile A pack
		if (WS && WAddrA < 32 && !(Flags & IF_PMFIXED)) // Regfile A write and pack mode not frozen.
		{	PM = false;
			goto packOK;
		}
		if (Pack < P_8abcdS)
			Fail(MSG.MUL_ALU_PACK_REQUIRES_8BIT_SAT);
	}
 packOK:

	applyRot(val.rValue.Rotate);
	applyPackUnpack(val.rValue.Pack);
}

void AssembleInst::applyALUSource(exprValue val)
{
	switch (val.Type)
	{default:
		Fail(MSG.ALU_SRC_NEEDS_REG_OR_SMI, type2string(val.Type));
	 case V_REG:
		setMux(muxReg(val.rValue));
		applyPackUnpack(val.rValue.Pack);
		applyRot(-val.rValue.Rotate);
		break;
	 case V_FLOAT:
	 case V_INT:
		qpuValue value = QPUValue(val);
		// some special hacks for ADD ALU
		if (InstCtx == (IC_SRCB|IC_ADD))
		{	switch (OpA)
			{case A_ADD: // swap ADD and SUB in case of constant 16 or negative SMI match
			 case A_SUB:
				if ( value.iValue == 16
					|| (Sig == S_SMI && SMIValue().iValue == -value.iValue) )
				{	(uint8_t&)OpA ^= A_ADD ^ A_SUB; // swap add <-> sub
					value.iValue = -value.iValue;
				}
				break;
			 case A_ASR: // shift instructions ignore all bits except for the last 5
			 case A_SHL:
			 case A_SHR:
			 case A_ROR:
				value.iValue = (value.iValue << 27) >> 27; // sign extend
			 default:;
			}
		}
		uint8_t si = AsSMIValue(value);
		if (si == 0xff)
			Fail(MSG.NO_SMI_VALUE, value.uValue);
		doSMI(si);
		setMux(X_RB);
	}
}

void AssembleInst::prepareMOV(bool target2)
{	if (Sig == S_BRANCH)
		Fail(MSG.NO_LDI_VS_BRANCH);
	bool isLDI = Sig == S_LDI;
	bool addused = (Flags & IF_HAVE_NOP) || WAddrA != R_NOP || (!isLDI && OpA != A_NOP);
	bool mulused = WAddrM != R_NOP || (!isLDI && OpM != M_NOP);
	if (target2)
	{	if (InstCtx != IC_BOTH)
			Fail(MSG.AMOV_MMOV_NO_2ND_TARGET);
		if (addused || mulused)
			Fail(MSG.TWO_TARGETS_REQUIRE_BOTH_ALU);
		goto addns;
	}
	if (isLDI && InstCtx != IC_BOTH)
		Fail(MSG.NO_AMOV_MMOV_VS_LDI);
	switch (InstCtx)
	{default:
		if (!addused)
			goto add;
		else if (!mulused)
			goto mul;
		else
			Fail(MSG.BOTH_ALU_IN_USE);
	 case IC_MUL:
		if (mulused && (addused || !tryALUSwap()))
			Fail(MSG.MUL_ALU_IN_USE);
		Flags |= IF_NOASWAP;
	 mul:
		InstCtx = IC_MUL|IC_OP;
		break;
	 case IC_ADD:
		if (addused && (mulused || !tryALUSwap()))
			Fail(MSG.ADD_ALU_IN_USE);
	 addns:
		Flags |= IF_NOASWAP;
	 add:
		InstCtx = IC_ADD|IC_OP;
		break;
	}
	atInitOP();
}

bool AssembleInst::applyMOVsrc(exprValue src)
{
	qpuValue value;
	switch (src.Type)
	{default:
		Fail(MSG.MOV_SRC_NEEDS_REG_OR_IMMD, type2string(src.Type));

	 case V_REG:
		if (src.rValue.Type & R_SEMA)
			return false; // requires LDI
		if (Sig == S_LDI)
			Fail(MSG.NO_MOV_REG_VS_LDI);

		setMux(muxReg(src.rValue));
		if (InstCtx & IC_MUL)
			OpM = M_V8MIN;
		else
			OpA = A_OR;
		applyPackUnpack(src.rValue.Pack);
		applyRot(-src.rValue.Rotate);
		return true;

	 case V_INT:
	 case V_FLOAT:
		value = QPUValue(src);
		// try small immediate first
		if ( Sig != S_LDI && (InstCtx & IC_BOTH) != IC_BOTH
			&& trySmallImmd(value.uValue))
			return true;
	 case V_LDPES:
	 case V_LDPEU:
	 case V_LDPE:
		return false;
	}
}

void AssembleInst::applyLDIsrc(exprValue src, ldmode mode)
{
	qpuValue value;
	switch (src.Type)
	{case V_REG:
		if ((src.rValue.Type & R_SEMA))
		{	if (src.rValue.Pack)
				Fail(MSG.NO_UNPACK_VS_BRANCH_LDI);
			// semaphore access by LDI like instruction
			if (!(mode & L_SEMA))
				(uint8_t&)mode |= L_SEMA;
			else if (!(mode & 0x80) ^ !(src.rValue.Type & R_SACQ))
				Fail(MSG.NO_SACQ_VS_SREL);
			value.uValue = src.rValue.Num | (src.rValue.Type & R_SACQ);
			break;
		}
	 default:
		Fail(MSG.MOV_SRC_NEEDS_REG_OR_IMMD, type2string(src.Type));

	 case V_LDPES:
		if (mode != L_LDI && mode != L_PES)
		 ldpe_fail:
			Fail(MSG.NO_LDI_VS_LDPE);
		mode = L_PES;
		goto ldpe_cont;
	 case V_LDPEU:
		if (mode != L_LDI && mode != L_PEU)
			goto ldpe_fail;
		mode = L_PEU;
		goto ldpe_cont;
	 case V_LDPE:
		if (mode & L_SEMA)
			goto ldpe_fail;
		if (mode != L_PEU)
			mode = L_PES;
	 ldpe_cont:
		value = QPUValue(src);
		break;

	 case V_FLOAT:
		if (mode & L_PEU)
			Fail(MSG.NO_LDPE_FLOAT);
	 case V_INT:
		value = QPUValue(src);
		if (mode & 0x80) // Acquire flag
			value.uValue |= 0x10;
		else if ((mode & L_SEMA) && (value.uValue & 0x10))
			Fail(MSG.NO_SREL_VS_IMMD_BIT4);
	}
	(uint8_t&)mode &= 7;

	switch (Sig)
	{default:
		Fail(MSG.NO_LDI_VS_SIG);
	 case S_SMI:
		Fail(MSG.NO_LDI_VS_SIG);
	 case S_LDI:
		if (Immd.uValue != value.uValue || LdMode != mode)
			Fail(MSG.NO_IMMD_VS_IMMD, Immd.uValue, value.uValue);
		break;
	 case S_NONE:
		if (OpA != A_NOP || OpM != M_NOP)
			Fail(MSG.NO_LDI_VS_ALU);
	}
	// LDI or semaphore
	Sig = S_LDI;
	LdMode = (ldmode)mode;
	Immd = value;
}

void AssembleInst::prepareREAD()
{	if (Sig == S_LDI || Sig == S_BRANCH)
		Fail(MSG.NO_READ_VS_BRANCH_LDI);
	InstCtx = IC_SRC;
	atInitOP();
}

void AssembleInst::applyREADsrc(exprValue src)
{	switch (src.Type)
	{default:
		Fail(MSG.READ_SRC_REQUIRES_REGFILE_OR_SMI, src.toString().c_str());
	 case V_REG:
		if (src.rValue.Rotate)
			Fail(MSG.NO_ROT_VS_READ);
		if (muxReg(src.rValue) <= X_R5)
			Fail(MSG.NO_ACCU_VS_READ);
		applyPackUnpack(src.rValue.Pack);
		break;
	 case V_INT:
	 case V_FLOAT:
		qpuValue value = QPUValue(src);
		uint8_t si = AsSMIValue(value);
		if (si == 0xff)
			Fail(MSG.NO_SMI_VALUE, value.uValue);
		doSMI(si);
	}
}

void AssembleInst::prepareBRANCH(bool relative)
{	if ( OpA != A_NOP || OpM != M_NOP || Sig != S_NONE
		|| RAddrA != R_NOP || RAddrB != R_NOP )
		Fail(MSG.NO_BRANCH_VS_ANY);
	InstCtx = IC_OP|IC_ADD;
	Sig = S_BRANCH;
	CondBr = B_AL;
	Rel = relative;
	RAddrA = 0;
	Reg = false;
	Immd.uValue = 0;
	atInitOP();
}

bool AssembleInst::applyBranchSource(exprValue val, unsigned pc)
{
	switch (val.Type)
	{default:
		Fail(MSG.BAD_BRANCH_SRC, type2string(val.Type));
	 case V_LABEL:
		if (Rel)
			val.iValue -= (pc + 4) * sizeof(uint64_t);
		else
			Msg(MSG.BRA_TO_LABEL);
		val.Type = V_INT;
	 case V_INT:
		if (Immd.uValue)
			Fail(MSG.NO_BRANCH_2ND_IMMD);
		if (!val.iValue)
			break;
		Immd = QPUValue(val);
		if (Immd.uValue & 3)
			Msg(MSG.BRANCH_TARGET_NOT_ALIGNED);
		break;
	 case V_REG:
		if (val.rValue.Num == R_NOP && !val.rValue.Rotate && (val.rValue.Type & R_AB))
			break;
		if (!(val.rValue.Type & R_A) || val.rValue.Num >= 32)
			Fail(MSG.BRANCH_SRC_REQIRES_REGA);
		if (val.rValue.Rotate)
			Fail(MSG.NO_ROT_VS_BRANCH);
		if (val.rValue.Pack)
			Fail(MSG.NO_UNPACK_VS_BRANCH_LDI);
		if (Reg)
			Fail(MSG.NO_BRANCH_2ND_REGA);
		if ((val.rValue.Num & 1) != SF)
		{	if (SF)
				Fail(MSG.NO_BRANCH_SETF_VS_EVEN_REG);
			else
				Msg(MSG.BRANCH_ODD_REG_IMPLIES_SETF);
		}
		Reg = true;
		RAddrA = val.rValue.Num;
		break;
	}

	return CondBr != B_AL || ((WAddrA != R_NOP || WAddrM != R_NOP) && (Immd.uValue || Reg));
}

void AssembleInst::applySignal(sig signal)
{
	switch (Sig)
	{default:
		Fail(MSG.NO_2ND_SIGNAL);
	 case S_BRANCH:
	 case S_LDI:
	 case S_SMI:
		Fail(MSG.NO_SIGNAL_VS_BRANCH_IMMD);
	 case S_NONE:
		Sig = signal;
	}
}

bool AssembleInst::tryRABSwap()
{	if ( !isALU()          // can't swap ldi and branch
		|| (Unpack && !PM)   // can't swap with regfile A unpack
		|| !isRRegAB(RAddrA) // regfile A read not invariant
		|| !isRRegAB(RAddrB) // regfile B read not invariant
		|| ( Sig == S_LDI    // can't swap small immediate, but vector rotation is OK
			&& (MuxAA == X_RB || MuxAB == X_RB || MuxMA == X_RB || MuxMB == X_RB) ))
		return false;
	// execute swap
	swap(RAddrA, RAddrB);
	if (MuxAA >= X_RA) (uint8_t&)MuxAA ^= X_RA^X_RB;
	if (MuxAB >= X_RA) (uint8_t&)MuxAB ^= X_RA^X_RB;
	if (MuxMA >= X_RA) (uint8_t&)MuxMA ^= X_RA^X_RB;
	if (MuxMB >= X_RA) (uint8_t&)MuxMB ^= X_RA^X_RB;
	return true;
}

bool AssembleInst::tryALUSwap()
{	if (Flags & IF_NOASWAP)
		return false;

	if ( !isALU()      // can't swap ldi and branch
		|| SF            // can't swap with set flags
		|| (Pack && PM)  // can't swap with MUL ALU pack
		|| (Sig == S_SMI && SImmd >= 48) ) // can't swap with vector rotation
		return false;
	opadd opa;
	switch (OpM)
	{default:
		return false;
	 case M_V8ADDS:
		opa = A_V8ADDS;
		break;
	 case M_V8SUBS:
		opa = A_V8SUBS;
		break;
	 case M_V8MIN:
	 case M_V8MAX:
		if (MuxMA != MuxMB)
			return false;
		opa = A_OR;
		break;
	 case M_NOP:
		opa = A_NOP;
		break;
	}
	opmul opm;
	switch (OpA)
	{default:
		return false;
	 case A_V8ADDS:
		opm = M_V8ADDS;
		break;
	 case A_XOR:
	 case A_SUB:
		if (MuxAA != MuxAB)
			return false;
	 case A_V8SUBS:
		opm = M_V8SUBS;
		break;
	 case A_OR:
	 case A_AND:
	 case A_MIN:
	 case A_MAX:
		if (MuxAA != MuxAB)
			return false;
		opm = M_V8MIN;
		break;
	 case A_NOP:
		opm = M_NOP;
		break;
	}
	// execute swap
	OpA = opa;
	OpM = opm;
	swap(MuxAA, MuxMA);
	swap(MuxAB, MuxMB);
	swap(CondA, CondM);
	swap(WAddrA, WAddrM);
	if ((InstCtx & IC_MUL) || WAddrA != R_NOP || !isWRegAB(WAddrM)) // nothing to swap?
		WS = !WS;

	InstCtx ^= IC_BOTH;
	if (UsePack)
		UsePack ^= IC_BOTH;
	if (UseUnpack)
		UseUnpack ^= IC_BOTH;
	if (UseRot)
		UseRot ^= IC_BOTH;
	return true;
}

bool AssembleInst::isTMUconflict() const
{	return isALU()
		&& ( ((0xfff09e0000000000ULL & (1ULL << WAddrA)) != 0)
		+ ((0xfff09e0000000000ULL & (1ULL << WAddrM)) != 0)
		+ ((0x0008060000000000ULL & (1ULL << RAddrA)) != 0)
		+ (Sig != S_SMI && (0x0008060000000000ULL & (1ULL << RAddrB)) != 0)
		+ ( Sig == S_LDTMU0 || Sig == S_LDTMU1
			|| Sig == S_LOADCV || Sig == S_LOADAM
			|| (Sig == S_LDI && (LdMode & L_SEMA)) )
		+ (WAddrA == 45 || WAddrA == 46 || WAddrM == 45 || WAddrM == 46 || Sig == S_LOADC || Sig == S_LDCEND) ) > 1;
}

void AssembleInst::optimize()
{	qpuValue val;
	switch (Sig)
	{case S_SMI:
		if ((1 << (Pack&15)) & 0x0909) // only pack modes that write the entire word
		{	if (WAddrM == R_NOP && OpM == M_NOP && MuxAA == X_RB && MuxAB == X_RB && !SF)
			{	// convert ADD ALU small immediate loads to LDI if MUL ALU is unused.
				val = SMIValue();
				if (evalADD(val, val) && evalPack(val, val, false))
					goto mkLDI;
			}
			if (OpA == A_NOP && MuxMA == X_RB && MuxMB == X_RB && !SF)
			{	// convert MUL ALU small immediate loads to LDI if ADD ALU is unused.
				val = SMIValue();
				if (evalMUL(val, val) && evalPack(val, val, true))
					goto mkLDI;
			}
		}
	 default:
		if (WAddrA == R_NOP && !SF)
			CondA = C_NEVER;
		switch (OpA)
		{case A_XOR:
		 case A_SUB:
			if (WAddrM == R_NOP && MuxAA == MuxAB)
				goto mkLDI0; // convert to LDI ..., 0
		 default:;
		}
		if (WAddrM == R_NOP && (!SF || OpA != A_NOP))
			CondM = C_NEVER;
		switch (OpM)
		{case M_V8SUBS:
			if (!SF && WAddrA == R_NOP && MuxMA == MuxMB)
				goto mkLDI0; // convert to LDI ..., 0
		 default:;
		}
		break;
	 mkLDI0:
		if (Sig != S_NONE || RAddrB != R_NOP)
			break;
		val.uValue = 0;
	 mkLDI:
		if (RAddrA != R_NOP)
			break;
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
