/*
 * Validator.cpp
 *
 *  Created on: 23.11.2014
 *      Author: mueller
 */

#include "Validator.h"
#include "DebugInfo.h"
#include <cstddef>
#include <cstring>
#include <cstdarg>


constexpr const struct ValidatorMSG Validator::MSG;


Validator::state::state(int start)
: From(start)
, Start(start)
{	fill_n(LastRreg[0], 128, NEVER);
	fill_n(LastWreg[0], 128, NEVER);
}
Validator::state::state(const state& r, int at, int target)
:	From(at)
,	Start(target)
{	// relocate Last...
	target -= at;
	const int* sp = &r.LastLDr4;
	int* dp = &LastLDr4;
	do
	{	*dp = *sp <= NEVER ? NEVER : *sp + target;
		++sp;
	} while (++dp < (int*)(this+1));
}

string Validator::Message::toString() const noexcept
{	string ret;
	if (Parent.Info)
	{	const auto loc = Parent.Info->LineNumbers[Loc];
		ret = stringf("%s (%u): ", Parent.Info->fName(loc.File), loc.Line);
	}
	ret += ::Message::toString();
	ret += stringf("\n  instruction at 0x%x", Parent.BaseAddr + Loc * sizeof(uint64_t));
	if (RefLoc != Loc)
	{	ret += stringf("\n  referring to instruction at 0x%x", Parent.BaseAddr + RefLoc * sizeof(uint64_t));
		if (Parent.Info)
		{	const auto loc = Parent.Info->LineNumbers[RefLoc];
			ret += stringf(", generated at %s (%u)", Parent.Info->fName(loc.File), loc.Line);
		}
	}
	return ret;
}

bool Validator::CheckMsg(int& refloc) const
{	if (refloc < Start)
		refloc += From - Start;
	else if (Pass2 && refloc >= Start)
		return false; // Discard message because of second pass.
	return true;
}


Inst::conda Validator::GetRdCond(Inst inst, Inst::mux m)
{
	switch (inst.Sig)
	{case Inst::S_LDI:
		// ldi never reads a register
		return Inst::C_NEVER;
	 case Inst::S_BRANCH:
		// branch is special, the branch conditions do not count here
		return inst.Reg && m == Inst::X_RA ? Inst::C_AL : Inst::C_NEVER;
	 case Inst::S_SMI:
		// small immediate cannot access regfile B
		if (m == Inst::X_RB)
			return Inst::C_NEVER;
	 default:;
	}
	Inst::conda res = Inst::C_NEVER;
	// ADD ALU read
	if (inst.MuxAA == m || (inst.MuxAB == m && !inst.isUnary()))
		res = inst.isSFADD() ? Inst::C_AL : inst.CondA;
	// MUL ALU read
	if (inst.MuxMA == m || inst.MuxMB == m)
	{	auto res2 = inst.isSFMUL() ? Inst::C_AL : inst.CondM;
		// merge conditions
		if (res == Inst::C_NEVER)
			res = res2;
		else if (res != res2 && res2 != Inst::C_NEVER)
			res = Inst::C_AL; // All non equal conditions except for C_NEVER have some overlap.
	}
	return res;
}

Inst::conda Validator::GetWrCond(Inst inst, uint8_t reg, regType type)
{	if ( inst.Sig == Inst::S_BRANCH
		|| ((type & R_AB) && inst.WAddrA == inst.WAddrM) )
		return Inst::C_AL;
	uint8_t mask = inst.WS ? R_B : R_A;
	return (mask & type) && reg == inst.WAddrA ? inst.CondA : inst.CondM;
}

void Validator::Decode(int refloc)
{	refloc = MakeAbsRef(refloc);
	if (refloc != RefDec)
	{	RefInst.decode((*Instructions)[refloc]);
		RefDec = refloc;
	}
}

void Validator::CheckRotSrc(const state& st, Inst::mux mux)
{	// check for back to back access in source register
	// This check is only required for accumulators since other registers cannot be accessed by the next instruction anyway
	// and r4/r5 does not support rotation between slices.
	if (!Inst::isAccu(mux) || st.LastWreg[0][32+mux] != At-1)
		return;
	// detailed check on current instruction
	if (CheckVectorRotationLevel > 1 || Instruct.isSFMUL())
	{warn:
		Msg(At-1, MSG.REG_WR_2_ROT_RD, mux);
		return;
	}
	if (Instruct.CondM == Inst::C_NEVER)
		return;
	if (Instruct.SImmd == 48) // rotate by r5
		goto warn;
	if (Instruct.SImmd >= 64-3 && Instruct.WAddrM == 32+5) // r5 write
		return;
	if (Instruct.CondM > Inst::C_AL) // conditional write might be OK
		return;
	// detailed check on reference instruction
	Decode(At-1);
	int reg = 32 + mux;
	int type = R_A * (RefInst.WAddrA == reg) | R_B * (RefInst.WAddrM == reg);
	Inst::conda wrcond = Inst::C_AL;
	if (RefInst.Sig != Inst::S_BRANCH)
	{	type &= ~(R_A * (RefInst.CondA == Inst::C_NEVER) | R_B * (RefInst.CondM == Inst::C_NEVER));
		switch (type)
		{default: // no write to the register
			return;
		 case R_A:
			wrcond = RefInst.CondA;
			break;
		 case R_B:
			wrcond = RefInst.CondM;
		 case R_AB:;
		}
	}
	// conditional write might also work
	if (wrcond == Inst::C_AL)
		goto warn;
}

void Validator::TerminateRq(int after)
{	//printf("TerminateRq %u, %x\n", after, after + At);
	after += At;
	if (after < To)
		To = after;
}

void Validator::ProcessItem(state& st)
{
	From = st.From;
	Start = st.Start;
	To = Instructions->size();
	//printf("Fragment %x, %x\n", From, Start);
	Pass2 = false;
	int target = -1;
	for (At = Start; At < To; ++At)
	{	if (Done[At])
		{	TerminateRq(MAX_DEPEND);
			Pass2 = true;
		}
		Instruct.decode((*Instructions)[At]);
		// Check resources
		uint8_t regRA = Inst::R_NOP;
		uint8_t regRB = Inst::R_NOP;
		uint8_t regWA = Instruct.WAddrA;
		uint8_t regWB = Instruct.WAddrM;
		bool ldr4 = false;
		bool tend = false;
		int rot = -1; // >= 0 = rotation active
		switch (Instruct.Sig)
		{case Inst::S_BRANCH:
			if (Instruct.Reg)
				regRA = Instruct.RAddrA;
			if (Instruct.CondBr == Inst::B_AL)
			{	TerminateRq(4);
				if (Instruct.WAddrA != Inst::R_NOP || Instruct.WAddrM != Inst::R_NOP)
					WorkItems.emplace_back(new state(At + 4));
			}
			if (At - st.LastBRANCH < 3)
				Msg(st.LastBRANCH, MSG.BRANCH_2_BRANCH);
			else if (!Instruct.Reg)
			{	target = Instruct.Rel
					?	Instruct.Immd.iValue / sizeof(uint64_t) + At + 4
					:	(Instruct.Immd.uValue - BaseAddr) / sizeof(uint64_t);
				st.LastBRANCH = At;
			}
			break;
		 case Inst::S_SMI:
			rot = Instruct.SImmd - 48;
			goto ALUA;
		 case Inst::S_THREND:
		 case Inst::S_LTHRSW:
			tend = true;
			goto ALU;
		 case Inst::S_LDCEND:
			tend = true;
		 case Inst::S_LDTMU0:
		 case Inst::S_LDTMU1:
		 case Inst::S_LOADCV:
		 case Inst::S_LOADC:
		 case Inst::S_LOADAM:
			ldr4 = true;
		 default:
		 ALU:
			regRB = Instruct.RAddrB;
		 ALUA:
			regRA = Instruct.RAddrA;
		 case Inst::S_LDI:
			if (Instruct.CondA == Inst::C_NEVER)
				regWA = Inst::R_NOP;
			if (Instruct.CondM == Inst::C_NEVER)
				regWB = Inst::R_NOP;
		}
		if (Instruct.WS)
			swap(regWA, regWB);

		// check for RA/RB back to back read/write
		if ( regRA < 32 && st.LastWreg[0][regRA] == At-1
			&& (Decode(At-1), IsCondOverlap(GetWrCond(RefInst, regRA, R_A), GetRdCond(Instruct, Inst::X_RA))) )
			Msg(At-1, MSG.REGA_WR_2_REGA_RD, regRA);
		if ( regRB < 32 && st.LastWreg[1][regRB] == At-1
			&& (Decode(At-1), IsCondOverlap(GetWrCond(RefInst, regRB, R_B), GetRdCond(Instruct, Inst::X_RB))) )
			Msg(At-1, MSG.REGB_WR_2_REGB_RD, regRB);
		// Two writes to the same register
		if ( regWA == regWB && regWA != Inst::R_NOP && Inst::isWRegAB(regWA)
			&& ( Instruct.Sig == Inst::S_BRANCH
				|| (Instruct.CondA != Inst::C_NEVER && Instruct.CondM != Inst::C_NEVER && Instruct.CondA != (Instruct.CondM ^ 1)) ))
			Msg(At, MSG.BOTH_ALU_WRITE_SAME_REG);
		if (At < 2 && Instruct.Sig == Inst::S_SBWAIT)
			Msg(At, MSG.SCOREBOARD_WR_AT_START);
		// unif_addr
		if (At - st.LastWreg[0][40] <= 2 && (regRA == 32 || regRB == 32))
			Msg(st.LastWreg[0][40], MSG.UNIF_ADDR_WR_2_UNIF_RD);
		if (regWA == 36 || regWB == 36)
		{	// TMU_NOSWAP
			for (int i = 56; i <= 63; ++i)
				if (st.LastWreg[0][i] >= 0)
				{	Msg(st.LastWreg[0][i], MSG.TMU_NOSWAP_WR_AFTER_TMU_RD);
					break;
				}
		}
		if (At - st.LastWreg[0][36] < 4 && (regWA >= 56 || regWB >= 56))
			Msg(st.LastWreg[0][36], MSG.TMU_NOSWAP_WR_2_TMU_RD);
		// r4
		int last = max(max(st.LastWreg[0][52], st.LastWreg[0][53]), max(st.LastWreg[0][54], st.LastWreg[0][55]));
		if (At - last <= 2)
		{	if (ldr4)
				Msg(last, MSG.SFU_OP_2_R4_WR_SIGNAL);
			if ((regWA & -4) == 52 || (regWB & -4) == 52)
				Msg(last, MSG.SFU_OP_2_SFU_OP);
		}
		// vector rotations
		if (CheckVectorRotationLevel && rot >= 0)
		{	// rot r5
			if (rot == 0 && st.LastWreg[0][32+5] == At-1)
				Msg(At-1, MSG.R5_WR_2_ROT_R5);
			CheckRotSrc(st, Instruct.MuxMA);
			if (Instruct.MuxMA != Instruct.MuxMB)
				CheckRotSrc(st, Instruct.MuxMB);

			// meaningless rotation
			if ( rot > 3 && rot < 16-3
				&& !Inst::isAccu(Instruct.MuxMA) && !Inst::isAccu(Instruct.MuxMB)
				&& Instruct.MuxAA != Inst::X_RB && Instruct.MuxAB != Inst::X_RB
				&& Instruct.MuxMA != Inst::X_RB && Instruct.MuxMB != Inst::X_RB )
				Msg(At, MSG.FULL_ROT_REQUIRES_ACCU);
		}
		// TLB Z -> MS_FLAGS
		if ((regRA == 42 || regRB == 42) && At - st.LastWreg[0][44] <= 2)
			Msg(st.LastWreg[0][44], MSG.TLB_Z_WR_2_MS_FLAGS_RD);
		// Combined peripheral access
		if (( ((0xfff09e0000000000ULL & (1ULL << regWA)) != 0) // TMU, TLB, SFU or Mutex write
			+ ((0xfff09e0000000000ULL & (1ULL << regWB)) != 0 && !(regWA == regWB && Inst::isWRegAB(regWA)))
			+ ((0x0008060000000000ULL & (1ULL << regRA)) != 0)   // TLB or Mutex read
			+ ((0x0008060000000000ULL & (1ULL << regRB)) != 0 && !(regRA == regRB && Inst::isRRegAB(regRA)))
			+ ( (Instruct.Sig >= Inst::S_LOADCV && Instruct.Sig <= Inst::S_LOADAM) // TLB read
				|| (Instruct.Sig == Inst::S_LDI && (Instruct.LdMode & Inst::L_SEMA)) ) // Semaphore access
			+ (regWA == 45 || regWA == 46 || regWB == 45 || regWB == 46 || Instruct.Sig == Inst::S_LOADC || Instruct.Sig == Inst::S_LDCEND) ) > 1 ) // combined TLB color read/write
			Msg(At, MSG.MULTI_ACCESS_2_TMU_TLB_SFU_MTX);
		// combined VPM access
		if (regWA == 49 && regWB == 49)
			Msg(At, MSG.VPM_WR_SETUP_VS_VPM_RD_SETUP);
		else if ((regWA == 49 || regWB == 49) && (regRA == 48 || regRB == 48))
			Msg(At, MSG.VPM_RD_VS_VPM_SETUP);
		if ((regWA == 49 && regWB == 48) || (regWB == 49 && regWA == 48))
			Msg(At, MSG.VPM_WR_VS_VPM_SETUP);
		// Some combinations that do not work
		if (Instruct.Sig != Inst::S_BRANCH)
		{	if ( !(Instruct.WAddrA == Instruct.WAddrM && (Instruct.CondA ^ Instruct.CondM) == 1) // No problem if both ALUs write conditionally to the same register with opposite conditions.
				&& ( (Instruct.CondA != Inst::C_AL && Inst::isPeripheralWReg(Instruct.WAddrA))
					|| (Instruct.CondM != Inst::C_AL && Inst::isPeripheralWReg(Instruct.WAddrM)) ))
				Msg(At, MSG.COND_WR_2_PERIPHERAL);
			if (Instruct.PM && (Instruct.Pack & 0xc) == Inst::P_8a && Inst::isPeripheralWReg(Instruct.WAddrM))
				Msg(At, MSG.PARTIAL_WR_2_PERIPHERAL);
		}
		// Update last used fields
		st.LastRreg[0][regRA] = At;
		st.LastRreg[!Inst::isRRegAB(regRB)][regRB] = At;
		st.LastWreg[0][regWA] = At;
		st.LastWreg[((1ULL<<regWB) & 0xfff9f9ff00000000ULL) == 0][regWB] = At;
		if (ldr4)
			st.LastLDr4 = At;
		if (tend && st.LastTEND < 0)
			st.LastTEND = At;
		if (Instruct.Sig == Inst::S_BRANCH && st.LastBRANCH < At -3)
			st.LastBRANCH = At;

		// Check for UNIF, VARY or VPM access after TEND
		if (st.LastTEND >= 0)
		{	if ( (((1ULL<<regRA)|(1ULL<<regRB)) & 0x0007000900000000ULL)
				|| (((1ULL<<regWA)|(1ULL<<regWB)) & 0x0007000000000000ULL) )
				Msg(st.LastTEND, MSG.UNIF_VARY_VPM_AFTER_THREND);
			if (regRA == 14 || regRB == 14 || regWA == 14 || regWB == 14)
				Msg(st.LastTEND, MSG.R14_AFTER_TRHEND);
		}
		if (tend && (regWA < 32 || regWB < 32))
			Msg(At, MSG.REG_WR_AT_THREND);
		if (At - st.LastTEND == 2 && (regWA == 44 || regWB == 44))
			Msg(st.LastTEND, MSG.TLBZ_WR_AT_END);

		if (tend)
			TerminateRq(3);
		if (!Pass2 && At - st.LastBRANCH == 3)
		{	// Schedule branch target
			if ((unsigned)target < Instructions->size() && !tend && st.LastTEND < 0)
				WorkItems.emplace_back(new state(st, At+1, target));
		}

		Done[At] = true;
	}
}

void Validator::Validate()
{
	RefDec = -1; // invalidate RefInst
	Done.clear();
	Done.resize(Instructions->size());

	if (Info)
	{	for (auto& seg : Info->Segments)
		{	//printf("VS: %u, %x\n", seg.Start, seg.Flags);
			if (seg.Flags == DebugInfo::SF_Code)
				WorkItems.emplace_back(new state(seg.Start));
			else if (seg.Flags == DebugInfo::SF_Data)
				Done[seg.Start] = true;
		}
	} else
		WorkItems.emplace_back(new state(0));

	while (!WorkItems.empty())
	{	unique_ptr<state> item = move(WorkItems.back());
		WorkItems.pop_back();
		ProcessItem(*item);
	}
}
