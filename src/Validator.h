/*
 * Validator.h
 *
 *  Created on: 23.11.2014
 *      Author: mueller
 */

#ifndef VALIDATOR_H_
#define VALIDATOR_H_

#include "expr.h"
#include "utils.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <climits>
using namespace std;

struct DebugInfo;

/// Type of the failed validation, for error-hooks
enum class FailureType
{
	/// write to the source of a vector-rotation in the previous instruction
	VECTOR_ROTATION_WRITE_SOURCE,
	/// a vector-rotation by r5 immediately follows a write to r5
	VECTOR_ROTATION_WRITE_R5,
	/// inputs of MUL ALU can't handle the full vector-rotation specified
	MEANINGLESS_VECTOR_ROTATION,
	/// two branch instructions with less than 3 instructions in between
	BRANCH_DISTANCE,
	/// a register is read in the instruction directly following a write to that register
	READ_AFTER_WRITE,
	/// both ALUs have the same output without inverted conditions (and neither is NOP)
	ALUS_SAME_OUTPUT,
	/// the first instructions of a shader access the scoreboard
	SHADER_START_SCOREBOARD_WAIT,
	/// a uniform is read less than 2 instructions after the uniform address is set
	UNIFORM_AFTER_UNIFORM_ADDRESS,
	/// TMU noswap is changed, after the TMU has been already used
	TMU_NOSWAP_AFTER_TMU_USE,
	/// write to TMU in the 3 instructions after write to TMU noswap
	TMU_AFTER_TMU_NOSWAP,
	/// a signal is omitted, which causes r4 to be filled, while an SFU-instruction is running
	SIGNAL_R4_WHILE_SFU,
	/// a request for the SFU is triggered while it is still processing another request
	SFU_IN_USE,
	/// multisample mask is read in the two instructions after a write to TLB Z
	MS_AFTER_TLB_Z,
	/// more than 1 access to periphery (SFU, TMU, TLB, mutex, semaphores) in one instruction
	MULTIPLE_ACCESS_PERIPHERY,
	/// conditional write to periphery
	CONDITIONAL_PERIPHERY,
	//writing into periphery with pack mode
	PACK_PERIPHERY,
	/// concurrent write to VPM read and VPM write setup registers
	CONCURRENT_VPM_SETUP,
	///concurrent read of VPM with write to VPM write setup
	CONCURRENT_VPM_READ,
	/// concurrent writing into VPM with write of VPM read setup
	CONCURRENT_VPM_WRITE,
	/// access to periphery (uniform, varyings, VPM) after thread-end
	THREAD_END_PERIPHERY,
	/// access to ra14 or rb14 after thread-end
	THREAD_END_R14,
	/// write to a register in the thread-end instruction
	THREAD_END_REGISTER,
	/// the last instruction writes to TLB Z
	THREAD_END_TLB_Z
};

/// @brief Hook to be notified when a validation fails
/// @param type The type of validation-error
/// @param pos The position (instruction-index) of the error
/// @param message The error-message
typedef void ValidationCallback(const FailureType type, int pos, const std::string& message);

/// Helper class to validate VideoCore IV instruction constraints.
class Validator
{public:
	/// Use this address as absolute address of the first instruction word.
	/// @remarks The absolute address is used for comments and to resolve absolute branch targets.
	uint32_t BaseAddr = 0;
	/// Code block to check. Need to be set before Validate().
	const vector<uint64_t>* Instructions = nullptr;
	/// Optional debug info for more expressive messages.
	const DebugInfo* Info = nullptr;

	/// Check register writes before vector rotations
	/// - 0 : no check
	/// - 1 : relaxed check, warn only combos that are likely to fail.
	/// - 2 : strict check, warn all cases.
	int CheckVectorRotationLevel = 1;

	/// An optional callback to be called when validations fail
	ValidationCallback* callback = nullptr;
 private:
	/// Maximum number of instructions where constraints apply.
	/// @remarks This is related to the pipeline length in QPU elements.
	/// It is used to reparse the constraints at branch targets.
	enum { MAX_DEPEND = 4 };
	/// Singleton constant to indicate that an instruction of this type had not yet been identified.
	/// @remarks This is just a very unlikely value.
	enum { NEVER = -0x40000000 };

	/// @brief Type to store integer values for each register in a regfile including I/O register.
	/// Index: [B!A][number].
	/// @details The first index selects regfile A/B. The second index the register number.
	/// @par For peripherals registers that are mapped to both register files
	/// the entries for register file A also track register file B access.
	/// The entries in [1] are unused in this case.
	typedef int regfile_t[2][64];
	/// @brief Validation worker state.
	/// @details The state saves all information about the current validator.
	/// It needs to be clonable to follow both paths at conditional branch instructions.
	struct state
	{	/// This work item is invoked from a branch from this location.
		/// 0 in case of the initial start address.
		const int From;
		/// Start instruction where the parsing started.
		const int Start;
		/// Last instruction that caused an load to register r4 by a signal (not SFU writes).
		int LastLDr4 = NEVER;
		/// Last instruction that was of type branch.
		int LastBRANCH = NEVER;
		/// Last thread end instruction.
		int LastTEND = NEVER;
		/// @brief Last register file read access at instruction #
		regfile_t LastRreg;
		/// @brief Last register file write access at instruction #
		regfile_t LastWreg;
		/// Create initial state.
		/// @param start First instruction to check. In doubt 0.
		state(int start);
		/// Revoke copy constructor.
		state(const state& r) = delete;
		/// Clone validator state.
		/// @param r State to clone
		/// @param at Clone is initiated from this instruction.
		/// @param target Further verification should start from here.
		/// @details The information about origin and target is used to relocate the last instruction fields.
		/// @remarks This constructor is intended for branch instructions.
		state(const state& r, int at, int target);
	};
	/// List of validator states. The list owns the states exclusively.
	typedef vector<unique_ptr<state>> workitems_t;
 private:
	/// @brief Bit vector of instructions that have already been checked.
	/// @details This is used to avoid redundant checks when code branches join.
	vector<bool> Done;
	/// @brief List of validations to be done.
	/// @details This list grows when branch instructions are processed
	/// and it shrinks as the items are processed.
	workitems_t WorkItems;
	// some fields are copied from *WorkItems.back() for efficiency reasons.
	int  From;        ///< Branch source, 0 in case of the initial start address.
	int  Start;       ///< Start instruction where the parsing started.
	int  At;          ///< Current Instruction.
	int  To;          ///< End analysis here.
	bool Pass2;       ///< Check only for dependencies of branch target.
	Inst Instruct;    ///< Current instruction to analyze.
	Inst RefInst;     ///< Reference instruction, decoded by Decode.
	int  RefDec;      ///< Decoded reference instruction.
 private:
	/// @brief Turn reference location into an absolute value.
	/// @details This is the opposite done in the \see state constructor.
	/// The value changes only when it is before \see Start.
	/// @param refloc location to relocate.
	/// @return Absolute index into instructions array.
	int  MakeAbsRef(int refloc) { if (refloc < Start) refloc += From - Start; return refloc; }
	/// @brief print a validation warning.
	/// @param refloc The Validation error at the current instruction (\ref At) is caused by the instruction at location \p refloc.
	/// The location is automatically relocated if it is from before a branch.
	/// @param fmt Message format string
	/// @remarks The reference locations before the branch have always instruction numbers less than start
	/// because the constructor relocated the accordingly. This function does the opposite transform to get meaningful messages.
	void Message(FailureType type, int refloc, const char* fmt, ...) PRINTFATTR(4);
	/// @brief Get effective condition of all read access to input mux \a m
	/// in the current instruction.
	/// @param inst Instruction to check.
	/// @param m Check only for access to this input mux.
	/// @details The function takes care of any ALU that uses this value
	/// for write to any register other than R_NOP or to the flags.
	static Inst::conda GetRdCond(Inst inst, Inst::mux m);
	/// Get effective write condition of RefInst for a certain register.
	/// @param inst Instruction to check.
	/// @param reg Register number to check.
	/// @param type Register file to check.
	/// @return Condition when writing to \a reg.
	/// @pre The instruction in RefIInst really writes to register \a reg.
	/// Otherwise the result is undefined.
	static Inst::conda GetWrCond(Inst inst, uint8_t reg, regType type);
	/// Check whether two conditions do overlap.
	static bool IsCondOverlap(Inst::conda cond1, Inst::conda cond2) { return (cond1|cond2) && (cond1^cond2) != 1; }
	/// Decode instruction at certain reference location into RefInst.
	/// @param refloc Location to decode. This is relocated before.
	/// @post RefInst gets the result.
	void Decode(int refloc);
	/// Check vector rotation for consistency.
	/// @param mux MUL ALU input multiplexer.
	/// @pre The current instruction must have a vector rotation
	/// and the previous instruction needs to write to \a mux if \a mux is an accumulator.
	void CheckRotSrc(const state& st, Inst::mux mux);
	/// Ensure termination of the current task.
	/// @param Maximum number of further instructions to check.
	void TerminateRq(int after);
	/// Execute a validation task for the given code block.
	/// @param instructions Code block to check.
	/// @param st (initial) state of the validator. This is also used to specify the region to be checked.
	void ProcessItem(state& st);
 public:
	/// Validate a code block. The validation starts always at the first instruction.
	/// @pre Instructions must be assigned to the code to analyze.
	void Validate();
};

#endif // VALIDATOR_H_
