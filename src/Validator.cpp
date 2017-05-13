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


void Validator::Message(FailureType type, int refloc, const char* fmt, ...)
{	if (refloc < Start)
		refloc += From - Start;
	else if (Pass2 && refloc >= Start)
		return; // Discard message because of second pass.
	va_list va;
	va_start(va, fmt);
	char buffer[1024];
	fputs("Warning: ", stderr);
	strcpy(buffer, "Warning: ");
	int bufPos = strlen("Warning: ");
	vfprintf(stderr, fmt, va);
	bufPos += vsprintf(buffer + bufPos, fmt, va);
	va_end(va);
	fprintf(stderr, "\n  instruction at 0x%x", BaseAddr + At * (unsigned)sizeof(uint64_t));
	bufPos += sprintf(buffer + bufPos, "\n  instruction at 0x%x", BaseAddr + At * (unsigned)sizeof(uint64_t));
	if (refloc >= 0 && refloc != At)
	{
		fprintf(stderr, " referring to instruction at 0x%x", BaseAddr + refloc * (unsigned)sizeof(uint64_t));
		bufPos += sprintf(buffer + bufPos, " referring to instruction at 0x%x", BaseAddr + refloc * (unsigned)sizeof(uint64_t));
	}
	fputc('\n', stderr);
	if (Info)
	{	auto loc = Info->LineNumbers[At];
		fprintf(stderr, "  generated at %s (%u)\n", Info->SourceFiles[loc.File].Name.c_str(), loc.Line);
		bufPos += sprintf(buffer + bufPos, ",  generated at %s (%u)\n", Info->SourceFiles[loc.File].Name.c_str(), loc.Line);
	}
	if(callback != nullptr)
	{
		callback(type, refloc, buffer);
	}
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
		Message(FailureType::VECTOR_ROTATION_WRITE_SOURCE, At-1, "Should not write to the source r%u of the vector rotation in the previous instruction.", mux);
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
				Message(FailureType::BRANCH_DISTANCE, st.LastBRANCH, "Two branch instructions within less than 3 instructions.");
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
			Message(FailureType::READ_AFTER_WRITE, At-1, "Cannot read register ra%d because it just has been written by the previous instruction.", regRA);
		if ( regRB < 32 && st.LastWreg[1][regRB] == At-1
			&& (Decode(At-1), IsCondOverlap(GetWrCond(RefInst, regRB, R_B), GetRdCond(Instruct, Inst::X_RB))) )
			Message(FailureType::READ_AFTER_WRITE, At-1, "Cannot read register rb%d because it just has been written by the previous instruction.", regRB);
		// Two writes to the same register
		if ( regWA == regWB && regWA != Inst::R_NOP && Inst::isWRegAB(regWA)
			&& ( Instruct.Sig == Inst::S_BRANCH
				|| (Instruct.CondA != Inst::C_NEVER && Instruct.CondM != Inst::C_NEVER && Instruct.CondA != (Instruct.CondM ^ 1)) ))
			Message(FailureType::ALUS_SAME_OUTPUT, At, "Both ALUs write to the same register without inverse condition flags.");
		if (At < 2 && Instruct.Sig == Inst::S_SBWAIT)
			Message(FailureType::SHADER_START_SCOREBOARD_WAIT, At, "The first two fragment shader instructions must not wait for the scoreboard.");
		// unif_addr
		if (At - st.LastWreg[0][40] <= 2 && (regRA == 32 || regRB == 32))
			Message(FailureType::UNIFORM_AFTER_UNIFORM_ADDRESS, st.LastWreg[0][40], "Must not read uniforms two instructions after write to unif_addr.");
		if (regWA == 36 || regWB == 36)
		{	// TMU_NOSWAP
			for (int i = 56; i <= 63; ++i)
				if (st.LastWreg[0][i] >= 0)
				{	Message(FailureType::TMU_NOSWAP_AFTER_TMU_USE, st.LastWreg[0][i], "Should not change tmu_noswap after the TMU has been used");
					break;
				}
		}
		if (At - st.LastWreg[0][36] < 4 && (regWA >= 56 || regWB >= 56))
			Message(FailureType::TMU_AFTER_TMU_NOSWAP, st.LastWreg[0][36], "Write to TMU must be at least 3 instructions after write to tmu_noswap.");
		// r4
		int last = max(max(st.LastWreg[0][52], st.LastWreg[0][53]), max(st.LastWreg[0][54], st.LastWreg[0][55]));
		if (At - last <= 2)
		{	if (ldr4)
				Message(FailureType::SIGNAL_R4_WHILE_SFU, last, "Cannot use signal which causes a write to r4 while an SFU instruction is in progress.");
			if ((regWA & -4) == 52 || (regWB & -4) == 52)
				Message(FailureType::SFU_IN_USE, last, "SFU is already in use.");
		}
		// vector rotations
		if (CheckVectorRotationLevel && rot >= 0)
		{	// rot r5
			if (rot == 0 && st.LastWreg[0][32+5] == At-1)
				Message(FailureType::VECTOR_ROTATION_WRITE_R5, At-1, "Vector rotation by r5 must not follow a write to r5.");
			CheckRotSrc(st, Instruct.MuxMA);
			if (Instruct.MuxMA != Instruct.MuxMB)
				CheckRotSrc(st, Instruct.MuxMB);

			// meaningless rotation
			if ( rot > 3 && rot < 16-3
				&& !Inst::isAccu(Instruct.MuxMA) && !Inst::isAccu(Instruct.MuxMB)
				&& Instruct.MuxAA != Inst::X_RB && Instruct.MuxAB != Inst::X_RB
				&& Instruct.MuxMA != Inst::X_RB && Instruct.MuxMB != Inst::X_RB )
				Message(FailureType::MEANINGLESS_VECTOR_ROTATION, At, "Neither MUL ALU source argument can handle full vector rotation.");
		}
		// TLB Z -> MS_FLAGS
		if ((regRA == 42 || regRB == 42) && At - st.LastWreg[0][44] <= 2)
			Message(FailureType::MS_AFTER_TLB_Z, st.LastWreg[0][44], "Cannot read multisample mask (ms_flags) in the two instructions after TLB Z write.");
		// Combined peripheral access
		if (( ((0xfff09e0000000000ULL & (1ULL << regWA)) != 0) // TMU, TLB, SFU or Mutex write
			+ ((0xfff09e0000000000ULL & (1ULL << regWB)) != 0 && !(regWA == regWB && Inst::isWRegAB(regWA)))
			+ ((0x0008060000000000ULL & (1ULL << regRA)) != 0)   // TLB or Mutex read
			+ ((0x0008060000000000ULL & (1ULL << regRB)) != 0 && !(regRA == regRB && Inst::isRRegAB(regRA)))
			+ ( (Instruct.Sig >= Inst::S_LOADCV && Instruct.Sig <= Inst::S_LOADAM) // TLB read
				|| (Instruct.Sig == Inst::S_LDI && (Instruct.LdMode & Inst::L_SEMA)) ) // Semaphore access
			+ (regWA == 45 || regWA == 46 || regWB == 45 || regWB == 46 || Instruct.Sig == Inst::S_LOADC || Instruct.Sig == Inst::S_LDCEND) ) > 1 ) // combined TLB color read/write
			Message(FailureType::MULTIPLE_ACCESS_PERIPHERY, At, "More than one access to TMU, TLB, SFU or mutex/semaphore within one instruction.");
		// combined VPM access
		if (regWA == 49 && regWB == 49)
			Message(FailureType::CONCURRENT_VPM_SETUP, At, "Concurrent write to VPM read and write setup does not work.");
		else if ((regWA == 49 || regWB == 49) && (regRA == 48 || regRB == 48))
			Message(FailureType::CONCURRENT_VPM_READ, At, "Concurrent VPM read with VPM setup does not work.");
		if ((regWA == 49 && regWB == 48) || (regWB == 49 && regWA == 48))
			Message(FailureType::CONCURRENT_VPM_WRITE, At, "Concurrent VPM write with VPM setup does not work.");
		// Some combinations that do not work
		if (Instruct.Sig != Inst::S_BRANCH)
		{	if ( !(Instruct.WAddrA == Instruct.WAddrM && (Instruct.CondA ^ Instruct.CondM) == 1) // No problem if both ALUs write conditionally to the same register with opposite conditions.
				&& ( (Instruct.CondA != Inst::C_AL && Inst::isPeripheralWReg(Instruct.WAddrA))
					|| (Instruct.CondM != Inst::C_AL && Inst::isPeripheralWReg(Instruct.WAddrM)) ))
				Message(FailureType::CONDITIONAL_PERIPHERY, At, "Conditional write to peripheral register does not work.");
			if (Instruct.PM && (Instruct.Pack & 0xc) == Inst::P_8a && Inst::isPeripheralWReg(Instruct.WAddrM))
				Message(FailureType::PACK_PERIPHERY, At, "Pack modes with partial writes do not work for peripheral registers.");
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
				Message(FailureType::THREAD_END_PERIPHERY, st.LastTEND, "Must not access uniform, varying or vpm register at thread end.");
			if (regRA == 14 || regRB == 14 || regWA == 14 || regWB == 14)
				Message(FailureType::THREAD_END_R14, st.LastTEND, "Must not access register 14 of register file A or B at thread end.");
		}
		if (tend && (regWA < 32 || regWB < 32))
			Message(FailureType::THREAD_END_REGISTER, At, "The thread end instruction must not write to either register file.");
		if (At - st.LastTEND == 2 && (regWA == 44 || regWB == 44))
			Message(FailureType::THREAD_END_TLB_Z, st.LastTEND, "The last program instruction must not write tlbz.");

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
