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


qpuValue AssembleInst::QPUValue(const exprValue& value)
{	qpuValue ret;
	switch (value.Type)
	{default:
		throw stringf("Value of type %s cannot be used for QPU evaluation. Only constants are allowed.", type2string(value.Type));

	 case V_LDPES:
	 case V_LDPE:
	 case V_LDPEU:
	 case V_INT:
		if (value.iValue < -0x80000000LL || value.iValue > 0xffffffffU)
			Fail("Integer constant 0x%" PRIx64 " out of range for use as QPU constant.", value.iValue);
		ret.iValue = (int32_t)value.iValue;
		break;

	 case V_FLOAT:
		if (fabs(value.fValue) > FLT_MAX && !std::isinf(value.fValue))
		{	Msg(WARNING, "Floating point constant %f does not fit into 32 bit float.", value.fValue);
			ret.fValue = value.fValue > 0 ? INFINITY : -INFINITY;
		} else
			ret.fValue = (float)value.fValue;
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
		Fail("Cannot use semaphore source in ALU or read instruction.");
	if (!(reg.Type & R_READ))
	{	// direct read access for r0..r5.
		if ((reg.Num ^ 32U) <= 5U)
			return (mux)(reg.Num ^ 32);
		Fail("The register is not readable.");
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
			Fail("Access to register file B conflicts with small immediate value.");
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
	Fail("Read access to register conflicts with another access to the same register file.");
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
		Fail("Small immediate value or vector rotation cannot be used together with signals.");
	 case S_SMI:
		if (SImmd == si)
			return; // value hit
		if ((si & 16) && ((SImmd ^ si) & 31) == 0)
		{	// Vector rotation codes return the same value than [16..31]
			// => only ensure the rotation bit
			SImmd |= si;
			return;
		}
		Fail("Only one distinct small immediate value supported per instruction. Requested value: %u, current Value: %u.", si, SImmd);
	 case S_NONE:
		if (RAddrB != R_NOP)
			Fail("Small immediate cannot be used together with register file B read access.");
	}
	Sig  = S_SMI;
	SImmd = si;
}

void AssembleInst::applyRot(int count)
{
	if (count)
	{	if (UseRot & (IC_OP | (InstCtx & IC_SRC)))
			Fail("Only one vector rotation per instruction, please.");

		if ((InstCtx & IC_BOTH) != IC_MUL && !tryALUSwap())
			Fail("Vector rotation is only available to the MUL ALU.");
		if (count == 16)
			Fail("Cannot rotate ALU target right by r5.");

		Flags |= IF_NOASWAP;
		bool isB = (InstCtx & IC_SRCB) != 0;
		auto mux = isB ? MuxMB : MuxMA;
		if (mux == X_R5)
			Msg(WARNING, "r5 does not support vector rotations.");

		count = 48 + (count & 0xf);
		auto isaccu = isAccu(mux);
		if (!isaccu && count > 48+3 && count < 64-3)
			Msg(WARNING, "%s does not support full MUL ALU vector rotation.", toString(mux));

		if (Sig == S_SMI)
		{	int mask = !isaccu
				|| ( isB && !isAccu(MuxMA)
					&& MuxAA != X_RB && MuxAB != X_RB
					&& MuxMA != X_RB && MuxMB != X_RB )
				? 0x13 : 0x1f;
			if ((SImmd ^ count) & mask)
				Fail( SImmd < 48
					? "Vector rotation is in conflict with small immediate value."
					: "Cannot use different vector rotations within one instruction." );

			if (!isaccu)
				count = SImmd | 0x20;
			else
				SImmd = count;
		}

		if ( (InstCtx & IC_SRC) == IC_SRCB
			&& (Sig != S_SMI || SImmd < 48)
			&& (isAccu(MuxMA) || (count & 0x3) || !count) )
			Msg(WARNING, "The Vector rotation of the second MUL ALU source argument silently applies also to the first source.");

		UseRot |= InstCtx;
		doSMI(count);

	} else if ( (InstCtx & IC_SRCB) && (UseRot & IC_SRCA)
			&& (isAccu(MuxMB) || (SImmd & 0x3) || SImmd == 48) )
		Msg(WARNING, "The vector rotation of the first MUL ALU source argument silently applies also to the second argument.");
}

void AssembleInst::applyPackUnpack(rPUp mode)
{
	switch (mode.requestType())
	{case rPUp::UNPACK:
		if (InstCtx & IC_DST)
			Fail("Unpack cannot be used in target context.");
	 unpack:
		doUnpack(mode.asUnPack());
		break;

	 case rPUp::PACK:
		if (InstCtx & IC_SRC)
			Fail("Register pack cannot be used in source context.");
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
		return 0; // regfile A pack only
	if (mode & P_FLT)
		return 1; // MUL ALU pack only
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
		return false;
	// Additionally check whether current ALU supports float modes.
	if ((mode & U_FLT) && !isFloatInput(InstCtx & IC_MUL))
		return false;
	return -1;
}

void AssembleInst::checkUnpack()
{	if ( !(InstCtx & IC_SRC)     // only check in source context
		|| !Unpack                 // no unpack, no problem
		|| PM                      // no ambiguities with r4 unpack
		|| (Unpack & 7) == U_8dr ) // no ambiguities with .8dr
		return;
	bool floatin = isFloatInput(InstCtx & IC_MUL);
	switch (Unpack & (U_INT|U_FLT))
	{case 0: // no explicit int/float mode, but lets nail the mode to avoid conflicts with the other ALU
		Unpack = (unpack)(Unpack | (floatin ? U_FLT : U_INT));
		break;
	 case U_INT:
		if (floatin)
			Fail("The requested unpack mode is only supported by instructions that take integer input.");
		break;
	 case U_FLT:
		if (!floatin)
		{	// if this is a mov instruction, executed by the ADD ALU then we could fulfill the request by changing the opcode
			if ( (InstCtx & IC_SRC) == IC_SRC
				&& ((InstCtx & IC_ADD) || tryALUSwap())
				&& OpA == A_OR )
				OpA = A_FMIN;
			else
				Fail("The requested unpack mode is only supported by instructions that take floating point input.");
		}
	}
	// Stage 2: check the other ALU
	bool otherfloatin;
	if (InstCtx & IC_MUL)
	{	if (!isADD())
			return; // ADD ALU unused => no conflict
		auto mux = currentMux();
		if (MuxAA != mux && MuxAB != mux)
			return; // ADD ALU does not access unpack mux => no conflict
		otherfloatin = isFloatInput(false);
	} else if (InstCtx & IC_ADD)
	{	if (!isADD())
			return; // MUL ADD ALU unused => no conflict
		auto mux = currentMux();
		if (MuxMA != mux && MuxMB != mux)
			return; // MUL ALU does not access unpack mux => no conflict
		otherfloatin = isFloatInput(true);
	}
	if (otherfloatin == !floatin)
	{	if (floatin)
			Fail("Using unpack with floating point input changes the behavior of the unpack operation of the other ALU.");
		else
			Fail("Unpack of the other ALU with a floating point instruction prevents access to the integer unpack modes.");
	}
}

void AssembleInst::applyPM(bool pm)
{	if (PM == pm)
		return; // match
	if (Pack)
		Fail("Required unpack mode conflicts with pack mode of this instruction.");
	if (Unpack && (UseUnpack & ~InstCtx & (IC_SRC|IC_BOTH)))
		Fail("Required unpack mode conflicts previous unpack mode.");
	PM = pm;
}

void AssembleInst::doUnpack(unpack mode)
{	int pm;
	if (!mode)
	{	// no unpack request at source context...
		if (UseUnpack & IC_OP)
		{	// OP code level unpack at current instruction...
			pm = isUnpackable(currentMux());
			// opcode unpack applies? => adjust PM
			int pm2 = calcPM(Unpack);
			if (pm >= 0 && pm2 < 0)
			{	if ((InstCtx & IC_SRC) == IC_SRCB && isUnpackable(InstCtx & IC_MUL ? MuxMA : MuxAA) == !pm)
					Fail("Ambiguous (un)pack mode. Unpack could apply to both source arguments.");
				applyPM((bool)pm);
				checkUnpack();
				UseUnpack |= InstCtx;
			}
			// check 4 unused unpack extensions
			switch (InstCtx & IC_BOTH)
			{case IC_ADD:
				if ( ((InstCtx & IC_SRCB) || isUnary()) // last source arg
						&& isUnpackable(MuxAA) != PM && isUnpackable(MuxAB) != PM )
					goto nounpack;
				break;
			 case IC_MUL:
				if ( (InstCtx & IC_SRCB) // last source arg
					&& isUnpackable(MuxMA) != PM && isUnpackable(MuxMB) != PM )
				 nounpack:
					Msg(WARNING, "The unpack option does not apply to any source argument.");
			 default:;
			}
		} else
		{	// no unpack at current context...
			// ensure that no unpack silently applies.
			if ((UseUnpack & (~InstCtx & IC_BOTH)) && PM == isUnpackable(currentMux()))
				Fail("The unpack option from the other ALU silently applies.");
			if ((InstCtx & IC_SRCB) && (UseUnpack & IC_SRCA) && PM == isUnpackable(currentMux()))
				Msg(WARNING, "The unpack option silently applies to 2nd source argument.");
		}
		return;
	}

	if (!isALU())
		Fail("Cannot apply .unpack to branch and load immediate instructions.");
	if (UseUnpack & (InstCtx|IC_OP) & (IC_OP|IC_SRC))
		Fail("Only one .unpack per ALU instruction, please.");
	// determine PM bit
	pm = calcPM(mode);
	if ((InstCtx & IC_SRC) && (InstCtx & IC_BOTH))
	{	int pm2 = isUnpackable(currentMux());
		if (pm2 < 0)
			Fail("Cannot unpack this source argument.");
		if ((!pm2) == pm)
			Fail("The requested unpack mode is not supported by this source argument.");
		pm = pm2;
	}
	// apply unpack mode to *this
	if (pm >= 0)
		applyPM((bool)pm);
	else if (!Pack && !Unpack)
		Msg(INFO, "Indeterminate unpack mode.");

	if (Unpack != U_32 && ((Unpack ^ mode) & 7))
		Fail("Cannot use different unpack modes within one instruction.");
	Unpack = mode;
	checkUnpack();
	UseUnpack |= InstCtx;
	// Check whether unpack applies unexpectedly to another argument.
	if ((InstCtx & IC_SRCB) && !(UseUnpack & (IC_OP|IC_SRCA)) && PM == isUnpackable(getMux(InstCtx ^ IC_SRC)))
		Msg(WARNING, "The unpack option silently applies to 1st source argument.");
	if ( (InstCtx | (IC_OP|IC_SRCA)) && ( InstCtx & IC_MUL
			? !(UseUnpack & IC_ADD) && isADD() && (isUnpackable(MuxAA) == PM || (!isUnary() && isUnpackable(MuxAB) == PM))
			: !(UseUnpack & IC_MUL) && isMUL() && (isUnpackable(MuxMB) == PM || isUnpackable(MuxMA) == PM) ))
		Fail("Using unpack changes the semantic of the other ALU.");
}

void AssembleInst::doPack(pack mode)
{
	if (!mode)
	{	if (!(InstCtx & IC_DST) || !(InstCtx & UsePack & IC_BOTH))
			return;
		mode = Pack;
	} else
	{	if (Sig == S_BRANCH)
			Fail("Cannot apply .pack to branch instruction.");
		if (Pack)
			Fail("Only one .pack per instruction, please.");
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
					Fail("Target of ADD ALU must be of regfile A to use pack.");
			}
			break; // apply PM

		 case true: // MUL ALU
			if (!(InstCtx & IC_MUL) && !tryALUSwap())
				Fail("The requested pack mode is only supported by the MUL ALU.");
		 case false:; // reg A
		}
		// apply PM
		if (pm != PM)
		{	if (Unpack)
				Fail("Required pack mode conflicts with unpack mode of this instruction.");
			PM = pm;
		}
	}
 applyPack:
	if ((InstCtx & IC_DST) && !PM)
	{	if (InstCtx & IC_MUL ? (!WS || WAddrM >= 32) : (WS || WAddrA >= 32))
			Fail("This pack mode requires regfile A target.");
		// check for int/float
		if ( (mode & (P_INT|P_FLT))
			&& (unsigned)(mode & (P_16a|P_16b|P_8a)) - P_16a <= P_16b - P_16a ) // 16 bit pack mode
		{	bool isfloat = isFloatResult(InstCtx & IC_MUL);
			if (!(mode & P_FLT) == isfloat)
			{	if (isfloat)
					Fail("This pack mode requires the result of an integer instruction.");
				/* TODO: in case of mov instruction and ADD ALU change opcode to fmin.
				else if ( (InstCtx & IC_SRC) == IC_SRC // hack for mov instruction
					&& ((InstCtx & IC_ADD) || (tryALUSwap() && OpA == A_OR)) )
					OpA = A_FMIN;*/
				else
					Fail("This pack mode requires the result of a floating point instruction.");
			}
		}
	}
	Pack = mode;
}

bool AssembleInst::trySmallImmd(uint32_t value)
{	const smiEntry* si;
	const bool wantmul = !!(InstCtx & IC_MUL);
	if (!value)
	{	// mov ... ,0 does not require small immediate value
		si = smiMap + wantmul;
	} else
	{	switch (Sig)
		{default:
			Fail("Immediate values cannot be used together with signals.");
		 case S_NONE:
			if (RAddrB != R_NOP)
				Fail("Immediate value collides with read from register file B.");
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
		Sig  = S_SMI;
		SImmd = si->SImmd;
		if (!!si->Pack)
		{	Pack = si->Pack;
			PM  = (si->Pack & P_FLT) != 0;
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
		Fail("Cannot apply conditional store (.ifxx) to branch instruction.");
	auto& target = InstCtx & IC_MUL ? CondM : CondA;
	if (target != C_AL)
		Fail("Store condition (.if) already specified.");
	target = cond;
}

void AssembleInst::applySetF()
{
	if ( Sig < S_LDI && (InstCtx & IC_MUL)
		&& (WAddrA != R_NOP || OpA != A_NOP)
		&& !tryALUSwap() )
		Fail("Cannot apply .setf because the flags of the ADD ALU will be used.");
	if (SF)
		Fail("Don't use .setf twice.");
	SF = true;
	Flags |= IF_NOASWAP;
}

void AssembleInst::applyCond(condb cond)
{
	if (Sig != S_BRANCH)
		Fail("Branch condition codes can only be applied to branch instructions.");
	if (CondBr != B_AL)
		Fail("Only one branch condition per instruction, please.");
	CondBr = cond;
}

int AssembleInst::applyADD(opadd add_op)
{	if (!isALU())
		Fail("Cannot use ADD ALU in load immediate or branch instruction.");
	if (isADD())
	{	if (isMUL())
		{fail:
			Fail("The ADD ALU has already been used in this instruction.");
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
	doInitOP();

	return add_op == A_NOP ? 0 : 1 + !isUnary();
}

int AssembleInst::applyMUL(opmul mul_op)
{	if (!isALU())
		Fail("Cannot use MUL ALU in load immediate or branch instruction.");
	if (isMUL())
	{	if (isADD())
		{fail:
			Fail("The MUL ALU has already been used by the current instruction.");
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
	doInitOP();

	return 2 * (mul_op != M_NOP);
}

void AssembleInst::applyTarget(reg_t reg)
{
	bool mul = (InstCtx & IC_MUL) != 0;
	if ((reg.Type & R_AB) != R_AB)
	{	bool wsfreeze = !isWRegAB(mul ? WAddrA : WAddrM); // Can't swap the other target register.
		if ((reg.Type & R_A) && (!wsfreeze || WS == mul))
			WS = mul;
		else if ((reg.Type & R_B) && (!wsfreeze || WS != mul))
			WS = !mul;
		else
			Fail("ADD ALU and MUL ALU cannot write to the same register file.");
	}
	(mul ? WAddrM : WAddrA) = reg.Num;
	// Check pack mode from /this/ opcode if any
	if ( Sig != S_BRANCH && Pack != P_32
		&& PM == mul && !(!mul && WS) ) // Pack matches my ALU and not RA pack of MUL ALU.
	{	if (!mul)
		{	if (!WS && WAddrA < 32)
				goto packOK;
			Fail("ADD ALU can only pack with regfile A target.");
		}
		// MUL ALU => prefer Regfile A pack
		if (WS && WAddrA < 32 && !Unpack) // Regfile A write and pack mode not frozen.
		{	PM = false;
			goto packOK;
		}
		if (Pack < P_8abcdS)
			Fail("MUL ALU only supports saturated pack modes with 8 bit.");
		// MUL ALU encodes saturated pack modes like unsaturated
		(uint8_t&)Pack &= 7;
	}
 packOK:

	applyRot(reg.Rotate);
	applyPackUnpack(reg.Pack);
}

void AssembleInst::applyALUSource(exprValue val)
{
	switch (val.Type)
	{default:
		Fail("The second argument of a binary ALU instruction must be a register or a small immediate value.");
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
			Fail("Value 0x%x does not fit into the small immediate field.", value.uValue);
		doSMI(si);
		setMux(X_RB);
	}
}

void AssembleInst::prepareMOV(bool target2)
{
	if (Sig == S_BRANCH)
		Fail("Cannot combine mov, ldi or semaphore instruction with branch.");
	bool isLDI = Sig == S_LDI;
	if (target2)
	{	if (InstCtx != IC_BOTH)
			Fail("amov/mmov cannot write two target registers.");
		if ( ((Flags & IF_HAVE_NOP) || WAddrA != R_NOP || (!isLDI && OpA != A_NOP))
			|| (WAddrM != R_NOP || (!isLDI && OpM != M_NOP)) )
			Fail("Instruction with two targets can only be used if both ALUs are available.");
		Flags |= IF_NOASWAP;
		InstCtx = IC_OP|IC_ADD;
	} else
	{	if (isLDI && InstCtx != IC_BOTH)
			Fail("Cannot combine amov, mmov with ldi or semaphore instruction.");
		bool addused = (Flags & IF_HAVE_NOP) || WAddrA != R_NOP || (!isLDI && OpA != A_NOP);
		bool mulused = WAddrM != R_NOP || (!isLDI && OpM != M_NOP);
		switch (InstCtx)
		{default:
			if (!addused)
				goto add;
			else if (!mulused)
				goto mul;
			else
				Fail("Both ALUs are already used by the current instruction.");
		 case IC_ADD:
			if (addused && (mulused || !tryALUSwap()))
				Fail("The ADD ALU has already been used in this instruction.");
			Flags |= IF_NOASWAP;
		 add:
			InstCtx = IC_ADD|IC_OP;
			break;
		 case IC_MUL:
			if (mulused && (addused || !tryALUSwap()))
				Fail("The MUL ALU has already been used in this instruction.");
			Flags |= IF_NOASWAP;
		 mul:
			InstCtx = IC_MUL|IC_OP;
			break;
		}
	}
	doInitOP();
}

bool AssembleInst::applyMOVsrc(exprValue src)
{
	qpuValue value;
	switch (src.Type)
	{default:
		Fail("The last parameter of a mov, ldi or semaphore instruction must be a register or a immediate value. Found %s", type2string(src.Type));

	 case V_REG:
		if (src.rValue.Type & R_SEMA)
			return false; // requires LDI
		if (Sig == S_LDI)
			Fail("mov instruction with register source cannot be combined with load immediate.");

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
	{default:
		Fail("The last parameter of a ldi or semaphore instruction must be an immediate value. Found %s", type2string(src.Type));

	 case V_REG:
		if (src.rValue.Pack)
			Fail("Unpack not allowed for ldi or semaphore instruction.");
		if (!(src.rValue.Type & R_SEMA))
			Fail("Register source not allowed for ldi or semaphore instruction.");
		// semaphore access by LDI like instruction
		if (!(mode & L_SEMA))
			(uint8_t&)mode |= L_SEMA;
		else if (!(mode & 0x80) ^ !(src.rValue.Type & R_SACQ))
			Fail("Mixing semaphore acquire and release.");
		value.uValue = src.rValue.Num | (src.rValue.Type & R_SACQ);
		break;

	 case V_LDPES:
		if (mode != L_LDI && mode != L_PES)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		mode = L_PES;
		goto ldpe_cont;
	 case V_LDPEU:
		if (mode != L_LDI && mode != L_PEU)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		mode = L_PEU;
		goto ldpe_cont;
	 case V_LDPE:
		if (mode & L_SEMA)
			Fail("Load immediate mode conflicts with per QPU element constant.");
		if (mode != L_PEU)
			mode = L_PES;
	 ldpe_cont:
		value = QPUValue(src);
		break;

	 case V_FLOAT:
		if (mode & L_PEU)
			Fail("Cannot load float value per element.");
	 case V_INT:
		value = QPUValue(src);
		if (mode & 0x80) // Acquire flag
			value.uValue |= 0x10;
		else if ((mode & L_SEMA) && (value.uValue & 0x10))
			Fail("Semaphore release instruction cannot deal with constants that have bit 4 set.");
	}
	(uint8_t&)mode &= 7;

	switch (Sig)
	{default:
		Fail("Load immediate or semaphore cannot be combined with signals.");
	 case S_SMI:
		Fail("This pair of immediate values cannot be handled in one instruction word.");
	 case S_LDI:
		if (Immd.uValue != value.uValue || LdMode != mode)
			Fail("Tried to load two different immediate values in one instruction. (0x%x vs. 0x%x)", Immd.uValue, value.uValue);
		break;
	 case S_NONE:
		if (OpA != A_NOP || OpM != M_NOP)
			Fail("Cannot combine load immediate with value %s with ALU instructions.", src.toString().c_str());
	}
	// LDI or semaphore
	Sig = S_LDI;
	LdMode = (ldmode)mode;
	Immd = value;
}

void AssembleInst::prepareREAD()
{	InstCtx = IC_SRC;
	if (Sig == S_LDI || Sig == S_BRANCH)
		Fail("read cannot be combined with load immediate, semaphore or branch instruction.");
	doInitOP();
}

void AssembleInst::applyREADsrc(exprValue src)
{
	switch (src.Type)
	{default:
		Fail("read instruction requires register file or small immediate source, found '%s'.", src.toString().c_str());
	 case V_REG:
		if (src.rValue.Rotate)
			Fail("Vector rotations cannot be used at read.");
		if (muxReg(src.rValue) <= X_R5)
			Fail("Accumulators cannot be used at read.");
		applyPackUnpack(src.rValue.Pack);
		break;
	 case V_INT:
	 case V_FLOAT:
		qpuValue value = QPUValue(src);
		uint8_t si = AsSMIValue(value);
		if (si == 0xff)
			Fail("Value 0x%" PRIx32 " does not fit into the small immediate field.", value.uValue);
		doSMI(si);
	}
}

void AssembleInst::prepareBRANCH(bool relative)
{	InstCtx = IC_OP|IC_ADD;

	if ( OpA != A_NOP || OpM != M_NOP || Sig != S_NONE
		|| RAddrA != R_NOP || RAddrB != R_NOP )
		Fail("A branch instruction must be the only one in a line.");

	Sig = S_BRANCH;
	CondBr = B_AL;
	Rel = relative;
	RAddrA = 0;
	Reg = false;
	Immd.uValue = 0;

	doInitOP();
}

bool AssembleInst::applyBranchSource(exprValue val, unsigned pc)
{
	switch (val.Type)
	{default:
		Fail("Data type is not allowed as branch target.");
	 case V_LABEL:
		if (Rel)
			val.iValue -= (pc + 4) * sizeof(uint64_t);
		else
			Msg(WARNING, "Using value of label as target of a absolute branch instruction creates non-relocatable code.");
		val.Type = V_INT;
	 case V_INT:
		if (Immd.uValue)
			Fail("Cannot specify two immediate values as branch target.");
		if (!val.iValue)
			break;
		Immd = QPUValue(val);
		if (Immd.uValue & 3)
			Msg(WARNING, "A branch target without 32 bit alignment probably does not hit the nail on the head.");
		break;
	 case V_REG:
		if (val.rValue.Num == R_NOP && !val.rValue.Rotate && (val.rValue.Type & R_AB))
			break;
		if (!(val.rValue.Type & R_A) || val.rValue.Num >= 32)
			Fail("Branch target must be from register file A and no hardware register.");
		if (val.rValue.Rotate)
			Fail("Cannot use vector rotation with branch instruction.");
		if (val.rValue.Pack)
			Fail("Cannot use unpack with branch instruction.");
		if (Reg)
			Fail("Cannot specify two registers as branch target.");
		if ((val.rValue.Num & 1) != SF)
		{	if (SF)
				Fail("Branch instruction with .setf cannot use even register numbers.");
			else
				Msg(WARNING, "Using an odd register number as branch target implies .setf. Use explicit .setf to avoid this warning.");
		}
		Reg = true;
		RAddrA = val.rValue.Num;
		break;
	}

	return (WAddrA != R_NOP || WAddrM != R_NOP) && (Immd.uValue || Reg);
}

void AssembleInst::applySignal(sig signal)
{
	switch (Sig)
	{default:
		Fail("You must not use more than one signaling flag per line.");
	 case S_BRANCH:
	 case S_LDI:
	 case S_SMI:
		Fail("Signaling bits cannot be combined with branch instruction or immediate values.");
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
			if (WAddrM == R_NOP && MuxAA == MuxAB)
				goto mkLDI0; // convert to LDI ..., 0
			break;
		}
		switch (OpM)
		{default:
			if (WAddrM != R_NOP || (SF && (OpA == A_NOP || CondA == C_NEVER)))
				break;
		 case M_NOP:
			if (WAddrM == R_NOP)
				CondM = C_NEVER;
			//MuxMA = X_R0;
			//MuxMB = X_R0;
			break;
		 case M_V8SUBS:
			if (!SF && WAddrA == R_NOP && MuxMA == MuxMB)
				goto mkLDI0; // convert to LDI ..., 0
			break;
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
