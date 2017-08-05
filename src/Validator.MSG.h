/*
 * Validator.MSG.h
 *
 *  Created on: 02.07.2017
 *      Author: mueller
 */


#ifndef VALIDATOR_MSG_H_
#define VALIDATOR_MSG_H_

#include <inttypes.h>

static const constexpr struct MSG
{
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Validator, WARN, major, minor }, text }

	W(BRANCH_2_BRANCH               , 10, 0, "Two branch instructions within less than 3 instructions.");

	W(REGA_WR_2_REGA_RD             , 20, 1, "Cannot read register ra%d because it just has been written by the previous instruction.", uint8_t);
	W(REGB_WR_2_REGB_RD             , 20, 2, "Cannot read register rb%d because it just has been written by the previous instruction.", uint8_t);
	W(REG_WR_2_ROT_RD               , 21,10, "Should not write to the source r%u of the vector rotation in the previous instruction.", Inst::mux);
	W(R5_WR_2_ROT_R5                , 21,20, 	"Vector rotation by r5 must not follow a write to r5.");
	W(FULL_ROT_REQUIRES_ACCU        , 22, 0, "Neither MUL ALU source argument can handle full vector rotation.");

	W(UNIF_ADDR_WR_2_UNIF_RD        , 30, 0, "Must not read uniforms two instructions after write to unif_addr.");
	W(TLB_Z_WR_2_MS_FLAGS_RD        , 31, 0, "Cannot read multisample mask (ms_flags) in the two instructions after TLB Z write.");

	W(TMU_NOSWAP_WR_2_TMU_RD        , 32,10, "Write to TMU must be at least 3 instructions after write to tmu_noswap.");
	W(TMU_NOSWAP_WR_AFTER_TMU_RD    , 32,20, "Should not change tmu_noswap after the TMU has been used");

	W(SFU_OP_2_SFU_OP               , 33,10, "SFU is already in use");
	W(SFU_OP_2_R4_WR_SIGNAL         , 33,20, "Cannot use signal which causes a write to r4 while an SFU instruction is in progress.");

	W(SCOREBOARD_WR_AT_START        , 35, 0, "The first two fragment shader instructions must not wait for the scoreboard.");
	W(UNIF_VARY_VPM_AFTER_THREND    , 40, 0, "Must not access uniform, varying or vpm register at thread end.");
	W(R14_AFTER_TRHEND              , 41, 0, "Must not access register 14 of register file A or B at thread end.");
	W(REG_WR_AT_THREND              , 42, 0, "The thread end instruction must not write to either register file.");
	W(TLBZ_WR_AT_END                , 45, 0, "The last program instruction must not write tlbz.");

	W(BOTH_ALU_WRITE_SAME_REG       ,100, 0, "Both ALUs write to the same register without inverse condition flags.");

	W(VPM_WR_SETUP_VS_VPM_RD_SETUP  ,110, 1, "Concurrent write to VPM read and write setup does not work.");
	W(VPM_RD_VS_VPM_SETUP           ,110, 2, "Concurrent write to VPM read and write setup does not work.");
	W(VPM_WR_VS_VPM_SETUP           ,110, 3, "Concurrent write to VPM read and write setup does not work.");

	W(MULTI_ACCESS_2_TMU_TLB_SFU_MTX,120, 0, "More than one access to TMU, TLB, SFU or mutex/semaphore within one instruction.");

	W(COND_WR_2_PERIPHERAL          ,130, 0, "Conditional write to peripheral register does not work.");
	W(PARTIAL_WR_2_PERIPHERAL       ,131, 0, "Pack modes with partial writes do not work for peripheral registers.");

	#undef W

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;

#endif // SRC_VALIDATOR_MSG_H_
