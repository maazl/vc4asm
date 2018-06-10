/*
 * Assemble.h
 *
 *  Created on: 19.06.2016
 *      Author: mueller
 */

#ifndef ASSEMBLEINST_H_
#define ASSEMBLEINST_H_

#include "utils.h"
#include "Message.h"
#include "expr.h"
#include "Inst.h"

#include <assert.h>


// Work around because gcc can't define a constexpr struct inside a class, part 1.
#define MSG AssembleInstMSG
#include "AssembleInst.MSG.h"
#undef MSG

/// Core class that does the assembly process.
class AssembleInst : private Inst
{public: // types...
	/// Context of a current expression during parse, bit vector.
	enum instContext : unsigned char
	{	IC_NONE     = 0x00   ///< No special context. No extensions allowed.
	,	IC_OP       = 0x01   ///< Op-code context, i.e. currently at the instruction itself.
	,	IC_SRCA     = 0x02   ///< Currently at the first source argument of an ADD ALU OP code.
	,	IC_SRCB     = 0x04   ///< Currently at the second source argument of an ADD ALU OP code.
	,	IC_SRC      = 0x06   ///< Bit test: currently at any source argument of an OP code; assignment: currently at both source arguments of an ADD ALU OP code.
	,	IC_DST      = 0x08   ///< Bit test: currently at the destination argument to an OP code; assignment: currently at the destination argument to an ADD ALU OP code.
	,	IC_ADD      = 0x10   ///< Bit test: currently at a ADD ALU instruction.
	,	IC_MUL      = 0x20   ///< Bit test: currently at a MUL ALU instruction.
	,	IC_BOTH     = 0x30   ///< Context of both ALUs, i.e branch or mov instruction with two targets.
	,	IC_XP       = 0x80   ///< Expression context, without IC_SRC or IC_DST it is indeterminate whether it becomes a source or a destination argument or whatever.
	};
	CLASSFLAGSENUM(instContext, unsigned char);

	/// Instruction optimization flags, bit vector
	enum instFlags : unsigned char
	{	IF_NONE          = 0    ///< no flags, none of the conditions below is met
	,	IF_HAVE_NOP      = 1    ///< at least one NOP in the current instruction so far
	,	IF_CMB_ALLOWED   = 2    ///< Instruction of the following line could be merged
	,	IF_BRANCH_TARGET = 4    ///< This instruction is a branch target and should not be merged
	,	IF_DATA          = 8    ///< Result of .data directive, do not optimize
	,	IF_NORSWAP       = 16   ///< Do not swap regfile A and regfile B peripheral register
	,	IF_NOASWAP       = 32   ///< Do not swap ADD and MUL ALU
	,	IF_PMFIXED       = 64   ///< PM bit should no longer be changed.
	};
	CLASSFLAGSENUM(instFlags, unsigned char);

 public: // messages
	/// Assembler message.
	/// The lifetime of this object and it's copies must not exceed the lifetime of the Validator instance that created the message.
	struct Message : public ::Message
	{	const AssembleInst& Parent; ///< Message source.
		Message(const AssembleInst& parent, msgID id, string&& text)
		: ::Message(id, move(text)), Parent(parent) {}
	 protected:
		Message(const AssembleInst& parent, ::Message&& msg) : ::Message(msg), Parent(parent) {}
		Message(const AssembleInst& parent, const ::Message& msg) : ::Message(msg), Parent(parent) {}
	};

	/// Message handler. Prints to \c stderr by default.
	function<void(Message&& msg)> OnMessage = Message::printHandler;

	// Work around because gcc can't define a constexpr struct inside a class, part 2.
	//#include "AssembleInst.MSG.h"
	static constexpr const struct AssembleInstMSG MSG = AssembleInstMSG;

 private:
	/// Reference to a ADD \e or MUL ALU operator.
	class opAddMul
	{	const int8_t   Op;      ///< ADD or MUL ALU opcode, bit 7 set: MUL ALU
	 public:
		/// Create from ADD ALU OP code.
		constexpr      opAddMul(opadd op) : Op(op) {}
		/// Create from MUL ALU OP code.
		constexpr      opAddMul(opmul op) : Op(op|0x80) {}
		/// true if the class instance represents a MUL ALU opcode
		/// false if it is an ADD ALU OP code.
		bool           isMul() const { return Op < 0; }
		/// Convert to Inst::opadd.
		/// @pre isMul() == false
		/// @return ADD ALU OP code.
		opadd          asAdd() const { return (opadd)Op; }
		/// Convert to Inst::opmul.
		/// @pre isMul() == true
		/// @return MUL ALU OP code.
		opmul          asMul() const { return (opmul)(Op & 0x7); }
	};
	/// Entry of the immediate value lookup table, POD, compatible with binary_search().
	static const struct smiEntry
	{	uint32_t       Value;   ///< Desired immediate value (result)
		uint8_t        SImmd;   ///< Small immediate required to achieve this result.
		opAddMul       OpCode;  ///< ALU opcode to achieve this result.
		pack           Pack;    ///< Pack mode to achieve the desired result.
	}	               smiMap[];///< Immediate value lookup table used for <tt>mov rx, immd</tt>.

 public:
	/// Current expression context.
	instContext      InstCtx;
	instFlags        Flags;

 private: // items valid per opcode...
	/// Pack mode used by the current instruction in this contexts.
	instContext      UsePack;
	/// Unpack mode used by the current instruction in this contexts.
	/// @details The Flags IC_SRCA and IC_SRCB will also be set in opcode
	instContext      UseUnpack;
	/// Vector rotation used by the current instruction in this contexts.
	instContext      UseRot;

 public:
	AssembleInst() {}
	virtual ~AssembleInst() {}

	void             reset() { Inst::reset(); UseUnpack = UsePack = IC_NONE; }

	/// Apply \c .if opcode extension
	/// @param cond Requested condition code.
	/// @exception Message Failed, error message.
	void             applyIf(conda cond);
	/// Apply \c .setf opcode extension
	/// @exception Message Failed, error message.
	void             applySetF();
	/// Apply \c branch condition code
	/// @param cond Condition code.
	/// @exception Message Failed, error message.
	void             applyCond(condb cond);
	/// Apply vector rotation to the current instruction context.
	/// @param count Desired rotation: 0 = none, 1 = one element right ... 15 = one element left, -16 = rotate by r5
	void             applyRot(int count);
	/// Apply pack or unpack to the current instruction context.
	/// @param mode Pack or unpack mode to apply.
	/// @exception Message Failed, error message.
	void             applyPackUnpack(rPUp mode);

	/// @brief Add ADD ALU op code to instruction word.
	/// @details Some instructions are available by both ALUs. applyADD tries to call applyMUL
	/// if such an instruction is encountered and the ADD ALU is already busy.
	/// @param op Operator for ADD ALU.
	/// @return Number of source arguments of the instruction, i.e. 0 for nop, 1 for unary op-codes and 2 for binary op-codes.
	/// @exception std::string Failed, error message.
	int              applyADD(opadd op);
	/// @brief Add MUL ALU op code to instruction word.
	/// @details Some instructions are available by both ALUs. applyMUL tries to call applyADD
	/// if such an instruction is encountered and the MUL ALU is already busy.
	/// @param op Operator for MUL ALU.
	/// @return Number of source arguments of the instruction, i.e. 0 for nop and 2 for binary op-codes.
	/// @exception std::string Failed, error message.
	int              applyMUL(opmul op);
	/// @brief Apply register as ALU target.
	/// @param val Register to write to.
	/// @pre The target ALU is taken from InstCtx. The ALU selected by InstCtx must be available.
	/// @exception Message Failed, error message.
	void             applyTarget(exprValue val);
	/// @brief Apply value as ALU source.
	/// @param val value.
	/// @pre Sig < S_LDI, the source multiplexer is taken from InstCtx.
	/// @exception Message Failed, error message.
	void             applyALUSource(exprValue val);
	/// @brief Prepare for mov or ldi instruction.
	/// @details Sets up instruction context and checks for ALU availability.
	/// @param target2 Flag whether the mov instruction wants to write both ALU targets.
	/// @pre InstCtx should contain the allowed ALUs, in doubt IC_ADD|IC_MUL.
	/// @exception std::string Failed, error message.
	void             prepareMOV(bool target2);
	/// @brief Handle Source of ALU move instruction.
	/// @param src Source expression
	/// @return true: everything OK, \a src accepted. false: This source requires LDI oder semaphore instruction. See applyLDIsrc.
	/// @exception std::string Failed, error message.
	bool             applyMOVsrc(exprValue src);
	/// @brief Handle source of LDI instruction, including semaphore.
	/// @param src Source expression. Must be a constant or a semaphore register.
	/// @param mode desired load mode. L_NONE in doubt.
	/// The mode is taken automatically in case of a register or per element source expression.
	/// Bit 7 is the acquire flag in case of L_SEMA.
	/// @exception std::string Failed, error message.
	void             applyLDIsrc(exprValue src, ldmode mode);

	void             prepareREAD();

	void             applyREADsrc(exprValue src);

	void             prepareBRANCH(bool relative);
	/// Handle source argument of branch instruction.
	/// @param val contains the source value to apply.
	/// @param pc Current program counter for calculation of label targets.
	/// @return Branch instruction is a conditional branch or likely to be a branch with link,
	/// i.e. the instruction after the branch point \c (pc+3) is likely to be a branch target.
	/// @exception Message Failed, error message.
	bool             applyBranchSource(exprValue val, unsigned pc);

	void             applySignal(sig signal);

	/// Check whether the current instruction has concurrent access to TMU resources.
	bool             isTMUconflict() const;

	/// Executed at each end of an ALU instruction.
	void             atEndOP();
	/*/// Executed at the end of each QPU instruction.
	void             atEndInst();*/

	/// Some optimizations to save ALU power
	void             optimize();

	using            Inst::encode;
	using            Inst::decode;

 protected: // messages
	/// Throw an exception message. You might override this function to throw something else.
	/// But whatever you implement must not return.
	[[noreturn]] virtual void ThrowMessage(Message&& msg) const;
	/// @brief Throw an assembler error.
	/// @param msg Message template
	template <typename ...A>
	[[noreturn]] void Fail(const msgTemplate<A...> msg, A... a) const
	{	ThrowMessage(Message(*this, msg.ID, msg.format(a...))); throw 0; /* unreachable code, but gcc will complain about the noreturn attribute otherwise. */ }
	/// @brief Create an assembler warning.
	/// @param msg Message template
	template <typename ...A>
	void Msg(const msgTemplate<A...> msg, A... a) const
	{	OnMessage(Message(*this, msg.ID, msg.format(a...))); }

 private:
	/// Fetch QPU value from vc4asm expression.
	/// @param value Value to convert, must be of type V_INT, V_FLOAT or V_LDPE*.
	/// @exception Message Failed, error message.
	qpuValue         QPUValue(const exprValue& value);
	/// @brief Find the first potential match for an ALU instruction that can assign an immediate value using small immediates.
	/// @details The function does not only seek for an exactly match small immediate value but also for small immediate values
	/// that	can be transformed to the wanted value by applying an ADD or MUL ALU instruction to it.
	/// The ALU instruction always takes the small immediate value for all of its arguments.
	/// @param i Requested immediate value.
	/// @return First potential match in the small immediate table.
	/// The table may have further potential matches in consecutive entries. So you need to seek
	/// until the Value property of the smiEntry changes. It is guaranteed that this happens before the end of the list.
	/// If no match is found the returned entry is still valid but it's Value property does not match.
	/// @remarks This function can significantly increase code density,
	/// because <tt>mov xx, immediate</tt> instructions often can be packed with other instruction this way.
	static const smiEntry* getSmallImmediateALU(uint32_t i);
	/// Return the input multiplexer of the current instruction that matches the given context.
	/// @param ctx ctx should contain one of IC_SRCAB and one of IC_BOTH.
	/// @return input multiplexer value
	mux              getMux(instContext ctx)
	{	assert((ctx & IC_SRC) && (ctx & IC_BOTH));
		switch (ctx & (IC_MUL|IC_SRCB))
		{case IC_NONE:
			return MuxAA;
		 case IC_SRCB:
			return MuxAB;
		 case IC_MUL:
			return MuxMA;
		 default:
			return MuxMB;
	}	};
	/// Return the input multiplexer of the current instruction that matches the current context.
	/// @pre InstCtx should contain one of IC_SRCAB and one of IC_BOTH.
	/// @return input multiplexer value
	mux              currentMux() { return getMux(InstCtx); }
	/// Assign the input multiplexer of the current instruction that matches the current context.
	/// @param val Multiplexer value
	/// @pre InstCtx should contain at least one of IC_SRCAB and one of IC_BOTH.
	void             setMux(mux val);
	/// @brief Get input mux for the current register expression register read access.
	/// @details Besides choosing the right multiplexer value this function also patches the register number
	/// into the current instruction and ensures that the value does not conflict with a previous usage of the same register file.
	/// While doing this job the function takes care of choosing regfile A or B respectively if the IO register
	/// is available in both register files. In doubt regfile A is preferred to keep the regfile B field free
	/// for small immediate values.
	/// @param reg Source register
	/// @return Matching multiplexer value.
	/// @exception std::string Error, i.e. no valid register or the current instruction cannot read this register because of conflicts
	/// with other parts of the same instruction word.
	Inst::mux        muxReg(reg_t reg);
	/// Set small immediate value. Fail if impossible.
	/// @param si desired value.
	/// @exception std::string Failed because of conflicts with other components of the current instruction word.
	void             doSMI(uint8_t si);
	/// Calculate PM bit from pack mode.
	/// @param mode Requested pack mode.
	/// @return PM bit or -1 in case of undetermined.
	/// @remarks Due to the capabilities of the QPUs all pack modes are deterministic if P_INT or P_FLT is set.
	static int       calcPM(pack mode);
	/// Calculate PM bit from unpack mode within the current context.
	/// @param mode Requested unpack mode.
	/// @return PM bit or -1 in case of undetermined.
	/// @remarks In fact the function will never return 1 because all r4 unpack modes are available by regfile A as well.
	static int       calcPM(unpack mode);
	static int       joinPM(int pm1, int pm2) { return (pm1 ^ pm2) == 1 ? -1 : pm1 & pm2; }
	/// Checks whether the given input mux may be the source of an unpack instruction, i.e. r4 or regfile A
	/// and if the requested int/float context is valid in this case.
	/// @param mux Input mux to check. If the mux points to regfile A The read address is also check not to address an IO register.
	/// @param mode Check for valid unpack mode.
	/// @return PM flag for the instruction, i.e. 1 for r4, 0 for regfile A, -1 if not possible or -5 in case of a int/float mismatch.
	int              isUnpackable(mux mux, unpack mode) const;
	/// Checks the unpack mode against the current instruction.
	/// Checks for conflicting int/float mode and may adjust opcode in case of mov instruction.
	/// @exception Message Failed, error message.
	void             checkUnpack();
	/// Adjust current pack mode and check for conflicts.
	/// @param @pm desired pack mode (PM)
	/// @exception Message Failed, error message.
	void             applyPM(bool pm);
	/// Handle unpack request at current context.
	/// @param mode Unpack mode to apply, might be none.
	/// @exception Message Failed, error message.
	void             doUnpack(unpack mode);
	/// Handle pack request at current context.
	/// @param mode Pack mode to apply, might be none.
	/// @exception Message Failed, error message.
	void             doPack(pack mode);

	/// Setup Parser for opcode arguments, handle instruction extensions.
	/// Called before any other function.
	void             atInitOP() { UseRot = IC_NONE; UseUnpack &= IC_BOTH; UsePack &= IC_BOTH; }

	/// Try to get immediate value at mov by a small immediate value and an available ALU.
	/// @param value requested immediate value.
	/// @return true: succeeded.
	/// @exception Message Failed, error message.
	bool             trySmallImmd(uint32_t value);
	/// Try to swap read access to register file A and B
	/// if the already existing read access is invariant of this change.
	/// See also isRRegAB.
	/// @return true: swap succeeded, false: instruction unchanged
	bool             tryRABSwap();
	/// Try to swap ADD and MUL ALU of current instruction if allowed by InstCtx, adjust InstCtx.
	/// @pre Instruct.Sig < Inst::S_BRANCH
	/// @return true: swap succeeded, false: instruction unchanged
	/// @post InstCtx & IC_MUL is inverted on success.
	bool             tryALUSwap();
};

#endif // ASSEMBLEINST_H_
