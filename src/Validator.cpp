/*
 * Validator.cpp
 *
 *  Created on: 23.11.2014
 *      Author: mueller
 */

#include "Validator.h"
#include <cstddef>
#include <cstring>
#include <cstdarg>


Validator::state::state()
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


void Validator::Message(int refloc, const char* fmt, ...)
{	if (refloc < Start)
		refloc += From - Start;
	else if (Pass2 && refloc >= Start)
		return; // Discard message because of second pass.
	va_list va;
	va_start(va, fmt);
	fputs("Warning: ", stderr);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n  instruction at 0x%x\n", BaseAddr + At * sizeof(uint64_t));
	if (refloc >= 0)
		fprintf(stderr, "  referring to instruction at 0x%x\n", BaseAddr + refloc * sizeof(uint64_t));
}

void Validator::TerminateRq(int after)
{	after += At;
	if (after < To)
		To = after;
}

void Validator::ProcessItem(const vector<uint64_t>& instructions, state& st)
{
	From = st.From;
	Start = st.Start;
	At = st.Start;
	To = instructions.size();
	Pass2 = false;
	int target = -1;
	for (At = st.Start; At < To; ++At)
	{	if (Done[At])
		{	TerminateRq(MAX_DEPEND);
			Pass2 = true;
		}
		Instruct.decode(instructions[At]);
		// Check resources
		uint8_t regRA = Inst::R_NOP;
		uint8_t regRB = Inst::R_NOP;
		uint8_t regWA, regWB;
		bool ldr4 = false;
		bool tend = false;
		bool rot = false;
		switch (Instruct.Sig)
		{case Inst::S_BRANCH:
			if (Instruct.Reg)
				regRA = Instruct.RAddrA;
			if (Instruct.CondBr == Inst::B_AL)
				TerminateRq(4);
			if (At - st.LastBRANCH < 4)
				Message(st.LastBRANCH, "Two branch instructions within less than 4 instructions.");
			else if (!Instruct.Reg)
			{	target = Instruct.Rel
					?	Instruct.Immd.iValue / sizeof(uint64_t) + At + 4
					:	(Instruct.Immd.uValue - BaseAddr) / sizeof(uint64_t);
				st.LastBRANCH = At;
			}
		 case Inst::S_LDI:
			break;
		 case Inst::S_SMI:
			rot = Instruct.SImmd >= 48;
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
		}
		if (Instruct.WS)
		{	regWA = Instruct.WAddrM;
			regWB = Instruct.WAddrA;
		} else
		{	regWA = Instruct.WAddrA;
			regWB = Instruct.WAddrM;
		}

		// check for RA/RB back to back read/write
		if (regRA < 32 && st.LastWreg[0][regRA] == At-1)
			Message(At-1, "Cannot read register ra%d because it just have been written by the previous instruction.", regRA);
		if (regRB < 32 && st.LastWreg[1][regRB] == At-1)
			Message(At-1, "Cannot read register rb%d because it just have been written by the previous instruction.", regRB);

		if (At < 2 && Instruct.Sig == Inst::S_SBWAIT)
			Message(At, "The first two fragment shader instructions must not wait for the scoreboard.");
		// unif_addr
		if (At - st.LastWreg[0][40] <= 2 && (regRA == 32 || regRB == 32))
			Message(st.LastWreg[0][40], "Must not read uniforms two instructions after write to unif_addr.");
		if (regWA == 36 || regWB == 36)
		{	// TMU_NOSWAP
			for (int i = 56; i <= 63; ++i)
				if (st.LastWreg[0][i] >= 0)
				{	Message(st.LastWreg[0][i], "Should not change tmu_noswap after the TMU has been used");
					break;
				}
		}
		if (At - st.LastWreg[0][36] < 4 && (regWA >= 56 || regWB >= 56))
			Message(st.LastWreg[0][36], "Write to TMU must be at least 3 instructions after write to tmu_noswap.");
		// r4
		int last = max(max(st.LastWreg[0][52], st.LastWreg[0][53]), max(st.LastWreg[0][54], st.LastWreg[0][55]));
		if (At - last <= 2)
		{	if (ldr4)
				Message(last, "Cannot use signal which causes a write to r4 while an SFU instruction is in progress.");
			if ((regWA & -4) == 52 || (regWB & -4) == 52)
				Message(last, "SFU is already in use.");
		}
		// rot r5
		if (rot && Instruct.SImmd == 48 && (st.LastWreg[0][37] == At-1 || st.LastWreg[1][37] == At-1))
			Message(At-1, "Vector rotation must not follow a write to r5.");
		if (st.LastRotReg >= 0 && (regWA == st.LastRotReg || regWB == st.LastRotReg))
			Message(At-1, "Must not write to the target of a vector in the following instruction.");
		// TLB Z -> MS_FLAGS
		if ((regRA == 42 || regRB == 42) && At - st.LastWreg[0][44] <= 2)
			Message(st.LastWreg[0][44], "Cannot read multisample mask (ms_flags) in the two instructions after TLB Z write.");
		// Combined peripheral access
		if (( ((0xfff09e0000000000ULL & (1ULL << regWA)) != 0)
			+ ((0xfff09e0000000000ULL & (1ULL << regWB)) != 0)
			+ ((0x0008060000000000ULL & (1ULL << regRA)) != 0)
			+ ((0x0008060000000000ULL & (1ULL << regRB)) != 0)
			+ ( Instruct.Sig == Inst::S_LDTMU0 || Instruct.Sig == Inst::S_LDTMU1
				|| Instruct.Sig == Inst::S_LOADCV || Instruct.Sig == Inst::S_LOADAM
				|| (Instruct.Sig == Inst::S_LDI && (Instruct.LdMode & Inst::L_SEMA)) )
			+ (regWA == 45 || regWA == 46 || regWB == 45 || regWB == 46 || Instruct.Sig == Inst::S_LOADC || Instruct.Sig == Inst::S_LDCEND) ) > 1 )
			Message(At, "More than one access to TMU, TLB or mutex/semaphore within one instruction.");

		// Update last used fields
		st.LastRreg[0][regRA] = At;
		st.LastRreg[!Inst::isRRegAB(regRB)][regRB] = At;
		st.LastWreg[0][regWA] = At;
		st.LastWreg[!Inst::isWRegAB(regWB)][regWB] = At;
		if (ldr4)
			st.LastLDr4 = At;
		if (tend && st.LastTEND < 0)
			st.LastTEND = At;
		if (Instruct.Sig == Inst::S_BRANCH && st.LastBRANCH < At -3)
			st.LastBRANCH = At;
		st.LastRotReg = rot ? Instruct.WAddrM : -1;

		// Check for UNIF, VARY or VPM access after TEND
		if (st.LastTEND >= 0)
		{	if ( (((1ULL<<regRA)|(1ULL<<regRB)) & 0x0007000900000000ULL)
				|| (((1ULL<<regWA)|(1ULL<<regWB)) & 0x0007000000000000ULL) )
				Message(st.LastTEND, "Must not access uniform, varying or vpm register at thread end.");
			if (regRA == 14 || regRB == 14 || regWA == 14 || regWB == 14)
				Message(st.LastTEND, "Must not access register 14 of register file A or B at thread end.");
		}
		if (tend && (regWA < 32 || regWB < 32))
			Message(At, "The thread end instruction must not write to either register file.");
		if (At - st.LastTEND == 2 && (regWA == 44 || regWB == 44))
			Message(st.LastTEND, "The last program instruction must not write tlbz.");

		if (tend)
			TerminateRq(3);
		if (!Pass2 && At - st.LastBRANCH == 3)
		{	// Schedule branch target
			if ((unsigned)target < instructions.size() && !tend && st.LastTEND < 0)
				WorkItems.emplace_back(new state(st, At+1, target));
		}

		Done[At] = true;
	}
}

void Validator::Validate(const vector<uint64_t>& instructions)
{
	Done.clear();
	Done.resize(instructions.size());
	WorkItems.emplace_back(new state);
	while (!WorkItems.empty())
	{	unique_ptr<state> item = move(WorkItems.back());
		WorkItems.pop_back();
		ProcessItem(instructions, *item);
	}
}
