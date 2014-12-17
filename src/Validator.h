/*
 * Validator.h
 *
 *  Created on: 23.11.2014
 *      Author: mueller
 */

#ifndef VALIDATOR_H_
#define VALIDATOR_H_

#include "Inst.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <climits>
using namespace std;

class Validator
{public:
	uint32_t BaseAddr = 0;
 private:
	/// Maximum number of instructions where constraints apply.
	enum { MAX_DEPEND = 4 };
	enum { NEVER = -0x40000000 };
	struct state
	{	/// This work item is invoked from a branch from this location.
		/// 0 in case of the initial start address.
		const int From = 0;
		/// Start instruction where the parsing started.
		const int Start = 0;
		int LastRotReg = -1;
		int LastLDr4 = NEVER;
		int LastBRANCH = NEVER;
		int LastTEND = NEVER;
		/// last register file read access at instruction #, [B!A][number]
		/// For peripherals registers that are mapped to both register files
		/// the entries for register file A also track register file B access.
		int LastRreg[2][64];
		/// last register file write access at instruction #, [B!A][number]
		/// For peripherals registers that are mapped to both register files
		/// the entries for register file A also track register file B access.
		int LastWreg[2][64];
		state();
		state(const state& r) = delete;
		state(const state& r, int at, int target);
	};
	typedef vector<unique_ptr<state>> workitems_t;
 private:
	vector<bool> Done;
	workitems_t WorkItems;
	int From;         ///< 0 in case of the initial start address.
	int Start;        ///< Start instruction where the parsing started.
	int At;           ///< Current Instruction.
	int To;           ///< End analysis here.
	bool Pass2;       ///< Check only for dependencies of branch target.
	Inst Instruct;
 private:
	void Message(int refloc, const char* fmt, ...);
	void TerminateRq(int after);
	void ProcessItem(const vector<uint64_t>& instructions, state& st);
 public:
	void Validate(const vector<uint64_t>& instructions);
};

#endif // VALIDATOR_H_
