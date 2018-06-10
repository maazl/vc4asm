/*
 * AssembleInst.MSG.h
 *
 *  Created on: 30.05.2017
 *      Author: mueller
 */

#include <inttypes.h>

static const constexpr struct MSG
{
	#define E(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Assembler, ERROR, major, minor }, text }
	#define W(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Assembler, WARN, major, minor }, text }
	#define I(name, major, minor, text, ...) const msgTemplate<__VA_ARGS__> name = { { MS_Assembler, INFO, major, minor }, text }
	// Entries must be ordered by major, minor.
	// Immediate arguments
	E(INT_OUT_OF_RANGE                , 10, 1, "Integer constant 0x%" PRIx64 " out of range for use as 32 bit QPU constant.", int64_t);
	W(FLOAT_OUT_OF_RANGE              , 10, 2, "Floating point constant %f does not fit into 32 bit float.", double);
	E(NO_SMI_VALUE                    , 11, 0, "Value 0x%" PRIx32 " does not fit into the small immediate field.", uint32_t);
	// Mutual exclusive access to register field
	E(NO_REG_VS_REG                   , 20, 0, "Read access to register conflicts with another access to the same register file.");
	E(NO_REGB_VS_SMI                  , 20,10, "Access to register file B conflicts with small immediate value.");
	E(NO_SMI_VS_SMI                   , 22, 1, "Only one distinct small immediate value supported per instruction. Requested value: %u, current Value: %u.", uint8_t, uint8_t);
	E(NO_SMI_VS_SIGNAL                , 22, 2, "Small immediate value or vector rotation cannot be used together with signals.");
	E(NO_SMI_VS_REGB                  , 22, 3, "Small immediate cannot be used together with register file B read access.");
	E(NO_IMMD_VS_IMMD                 , 23, 1, "Tried to load two different immediate values in one instruction. (0x%" PRIx32 " vs. 0x%" PRIx32 ")", uint32_t, uint32_t);
	E(NO_IMMD_VS_SIGNAL               , 23, 2, "Immediate values cannot be used together with signals.");
	E(NO_IMMD_VS_REGB                 , 23, 3, "Immediate value collides with read from register file B.");
	E(NO_ROT_VS_ROT                   , 24, 1, "Cannot use different vector rotations within one instruction.");
	E(NO_ROT_VS_BRANCH                , 25, 2, "Cannot use vector rotation with branch instruction.");
	E(NO_ROT_VS_SMI                   , 25, 3, "Vector rotation is in conflict with small immediate value.");
	// vector rotation
	E(NO_2ND_ROT                      , 30, 0, "Only one vector rotation per instruction, please.");
	E(ROT_REQIURES_MUL_ALU            , 31, 0, "Vector rotation is only available to the MUL ALU.");
	E(NO_ROTR_BY_R5                   , 32, 0, "Cannot rotate ALU target right by r5.");
	W(NO_ROT_OF_R5                    , 33, 0, "r5 does not support vector rotations.");
	W(NO_FULL_ROT                     , 35, 1, "%s does not support full MUL ALU vector rotation.", const char*);
	W(ROT_SRC2_APPLIES_TO_SRC1        , 36, 1, "The vector rotation of the second MUL ALU source argument silently applies also to the first source.");
	W(ROT_SRC1_APPLIES_TO_SRC2        , 36, 2, "The vector rotation of the first MUL ALU source argument silently applies also to the second argument.");
	// unpack
	E(NO_2ND_UNPACK                   , 40, 0, "Only one .unpack per ALU instruction, please.");
	E(NO_UNPACK_VS_BRANCH_LDI         , 41, 0, "Cannot apply .unpack to branch and load immediate instructions.");
	E(NO_UNPACK_TARGET                , 42, 0, "Unpack cannot be used in target context.");
	E(NO_UNPACK_SRC                   , 43, 1, "Cannot unpack this source argument.");
	E(NO_UNPACK_MODE_SRC              , 43,10, "The requested unpack mode is not supported by this source argument.");
	E(NO_UNPACK_TO_FLOAT              , 43,11, "The requested unpack mode is only supported by instructions that take integer input.");
	E(NO_UNPACK_TO_INT                , 43,12, "The requested unpack mode is only supported by instructions that take floating point input.");
	E(FLOAT_AFFECTS_OTHER_ALU_UNPACK  , 44, 1, "Using a floating point instruction changes the behavior of the unpack operation of the other ALU.");
	E(UNPACK_APPLIES_FORM_OTHER_ALU   , 45, 1, "The unpack option from the other ALU silently applies.");
	E(UNPACK_APPLIES_TO_OTHER_ALU     , 45, 2, "The unpack option silently applies to the other ALU instruction.");
	W(UNPACK_APPLIES_TO_SRC1          , 46, 1, "The unpack option silently applies to 1st source argument.");
	W(UNPACK_APPLIES_TO_SRC2          , 46, 2, "The unpack option silently applies to 2nd source argument.");
	E(AMBIGUOUS_UNPACK                , 47, 1, "Ambiguous unpack mode. Unpack could apply to both source arguments with different semantics.");
	I(INDETERMINATE_UNPACK            , 49, 2, "Ambiguous unpack mode. Trying regfile A unpack.");
	W(UNUSED_UNPACK                   , 48, 0, "The unpack option does not apply to any source argument.");
	// pack
	E(NO_2ND_PACK                     , 60, 0, "Only one .pack per instruction, please.");
	E(NO_PACK_BRANCH_LDI              , 61, 0, "Cannot apply .pack to branch instruction.");
	E(NO_PACK_SOURCE                  , 62, 0, "Register pack cannot be used in source context.");
	E(ADD_ALU_PACK_NEEDS_REGA         , 63,10, "Target of ADD ALU must be of register file A to use pack.");
	E(MUL_ALU_PACK_REQUIRES_8BIT_SAT  , 63,20, "MUL ALU only supports saturated pack modes with 8 bit.");
	E(PACK_MODE_REQUIRES_MUL_ALU      , 65,10, "The requested pack mode is only supported by the MUL ALU.");
	E(PACK_MODE_REQUIRES_REGA         , 65,11, "This pack mode requires register file A target.");
	E(PACK_MODE_REQUIRES_INT          , 65,20, "This pack mode requires the result of an integer instruction.");
	E(PACK_MODE_REQUIRES_FLOAT        , 65,21, "This pack mode requires the result of a floating point instruction.");
	// pack vs unpack
	E(NO_UNPACK_VS_UNPACK             , 70, 0, "Required unpack mode conflicts previous unpack mode.");
	E(NO_UNPACK_VS_PACK               , 71, 0, "Required unpack mode conflicts with pack mode of this instruction.");
	E(NO_PACK_VS_UNPACK               , 72, 0, "Required pack mode conflicts with unpack mode of this instruction.");
	// other extensions
	E(NO_2ND_SETF                     , 80, 0, "Don't use .setf twice.");
	E(NO_MUL_SETF_VS_ADD              , 81, 0, "Cannot apply .setf because the flags of the ADD ALU will be used.");
	E(NO_2ND_IFCC                     , 83, 0, "Store condition (.if) already specified.");
	E(NO_IFCC_VS_BRANCH               , 84, 0, "Cannot apply conditional store (.ifxx) to branch instruction.");
	E(NO_2ND_BCC                      , 86, 0, "Only one branch condition per instruction, please.");
	E(BCC_REQUIRES_BRANCH             , 87, 0, "Branch condition codes can only be applied to branch instructions.");
	// source
	E(MOV_SRC_NEEDS_REG_OR_IMMD       ,110, 1, "The last parameter of a mov, ldi or semaphore instruction must be a register or a immediate value. Found %s.", const char*);
	E(ALU_SRC_NEEDS_REG_OR_SMI        ,110, 2, "The second and third argument of a ALU instruction must be a register or a small immediate value. Found %s.", const char*);
	E(NO_SEMA_SOURCE                  ,111, 0, "Cannot use semaphore source in ALU or read instruction.");
	E(SRC_REGISTER_NO_READ            ,115, 0, "The register is not readable.");
	// target
	E(TARGET_NEEDS_REG                ,130, 0, "The target argument to an ALU or branch instruction must be a register or '-', found %s.", const char*);
	E(NO_SAME_REGFILE_WRITE           ,131, 0, "ADD ALU and MUL ALU cannot write to the same register file.");
	E(TARGET_REGISTER_NO_WRITE        ,135, 0, "Instruction with two targets can only be used if both ALUs are available.");
	E(TWO_TARGETS_REQUIRE_BOTH_ALU    ,136, 0, "Instruction with two targets can only be used if both ALUs are available.");
	E(AMOV_MMOV_NO_2ND_TARGET         ,137, 0, "amov/mmov cannot write two target registers.");
	// ALU
	E(NO_ADD_VS_BRANCH_LDI            ,100, 1, "Cannot use ADD ALU together with load immediate or branch instruction.");
	E(NO_MUL_VS_BRANCH_LDI            ,100, 2, "Cannot use MUL ALU together with load immediate or branch instruction.");
	E(ADD_ALU_IN_USE                  ,101, 1, "The ADD ALU has already been used in this instruction.");
	E(MUL_ALU_IN_USE                  ,101, 2, "The MUL ALU has already been used in this instruction.");
	E(BOTH_ALU_IN_USE                 ,102, 0, "Both ALUs are already used by the current instruction.");
	// mov, ldi, sema
	E(NO_MOV_REG_VS_LDI               ,120,10, "mov instruction with register source cannot be combined with load immediate.");
	E(NO_AMOV_MMOV_VS_LDI             ,120,20, "Cannot combine amov, mmov with ldi or semaphore instruction.");
	E(NO_LDPE_FLOAT                   ,122, 0, "Cannot load float value per element.");
	E(NO_LDI_VS_BRANCH                ,124, 1, "Cannot combine mov, ldi or semaphore instruction with branch.");
	E(NO_LDI_VS_SIG                   ,124, 2, "Load immediate or semaphore cannot be combined with signals.");
	E(NO_LDI_VS_SMI                   ,124, 3, "This pair of immediate values cannot be handled in one instruction word.");
	E(NO_LDI_VS_ALU                   ,124, 4, "Cannot combine load immediate with value with ALU instructions.");
	E(NO_LDI_VS_LDPE                  ,124,10, "Load immediate mode conflicts with per QPU element constant.");
	E(NO_SACQ_VS_SREL                 ,125, 1, "Mixing semaphore acquire and release.");
	E(NO_SREL_VS_IMMD_BIT4            ,125, 0, "Semaphore release instruction cannot deal with constants that have bit 4 set.");
	// read
	E(NO_READ_VS_BRANCH_LDI           ,140, 0, "read cannot be combined with load immediate, semaphore or branch instruction.");
	E(READ_SRC_REQUIRES_REGFILE_OR_SMI,141, 0, "read instruction requires register file or small immediate source, found '%s'.", const char*);
	E(NO_ROT_VS_READ                  ,142,10, "Vector rotations cannot be used at read.");
	E(NO_ACCU_VS_READ                 ,142,20, "Accumulators cannot be used at read.");
	// branch
	E(NO_BRANCH_VS_ANY                ,150, 0, "A branch instruction must be the only one in a line.");
	E(BAD_BRANCH_SRC                  ,151, 0, "Data type '%s' is not allowed as branch target.", const char*);
	E(BRANCH_SRC_REQIRES_REGA         ,152, 0, "Branch target must be from register file A and no hardware register.");
	E(NO_BRANCH_2ND_IMMD              ,153,10, "Cannot specify two immediate values as branch target.");
	E(NO_BRANCH_2ND_REGA              ,153,20, "Cannot specify two registers as branch target.");
	W(BRA_TO_LABEL                    ,155, 0, "Using value of label as target of a absolute branch instruction creates non-relocatable code.");
	W(BRANCH_TARGET_NOT_ALIGNED       ,156, 0, "A branch target without 32 bit alignment probably does not hit the nail on the head.");
	E(NO_BRANCH_SETF_VS_EVEN_REG      ,157, 1, "Branch instruction with .setf cannot use an even register number.");
	W(BRANCH_ODD_REG_IMPLIES_SETF     ,157, 2, "Using an odd register number as branch target implies .setf. Use explicit .setf to avoid this warning.");
	// signals
	E(NO_2ND_SIGNAL                   ,160, 0, "You must not use more than one signaling flag per instruction.");
	E(NO_SIGNAL_VS_BRANCH_IMMD        ,161, 0, "Signaling bits cannot be combined with branch instruction or immediate values.");

	#undef I
	#undef E
	#undef W

	MAKE_MSGTEMPLATE_CONTAINER
} MSG;
