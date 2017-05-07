/*
 * Parser.tables.cpp
 *
 *  Created on: 26.10.2014
 *      Author: mueller
 */

#include "Parser.h"

// MUST BE ORDERED!
const Parser::opInfo Parser::operatorMap[] =
{	{ "!",   Eval::lNOT }
,	{ "!=",  Eval::NE }
,	{ "!==", Eval::NIDNT }
,	{ "!^",  Eval::XNOR }
,	{ "!^^", Eval::lXNOR }
,	{ "%",   Eval::MOD }
,	{ "&",   Eval::AND }
,	{ "&&",  Eval::lAND }
,	{ "(",   Eval::BRO }
,	{ ")",   Eval::BRC }
,	{ "*",   Eval::MUL }
,	{ "**",  Eval::POW }
,	{ "+",   Eval::ADD } // also unary NOP
,	{ "-",   Eval::SUB } // also unary NEG
,	{ "/",   Eval::DIV }
,	{ "<",   Eval::LT }
,	{ "<<",  Eval::ASL }
,	{ "<<<", Eval::SHL }
,	{ "<=",  Eval::LE }
,	{ "<=>", Eval::CMP }
,	{ "==",  Eval::EQ }
,	{ "===", Eval::IDNT }
,	{ ">",   Eval::GT }
,	{ "><<", Eval::ROL32 }
,	{ "><<<",Eval::ROL64 }
,	{ ">=",  Eval::GE }
,	{ ">>",  Eval::ASR }
,	{ ">><", Eval::ROR32 }
,	{ ">>>", Eval::SHR }
,	{ ">>><",Eval::ROR64 }
,	{ "^",   Eval::XOR }
,	{ "^^",  Eval::lXOR }
,	{ "|",   Eval::OR }
,	{ "||",  Eval::lOR }
,	{ "~",   Eval::NOT }
};
// MUST BE ORDERED!
const Parser::opInfo Parser::operatorMap2[] =
{	{ "abs",   Eval::ABS }
,	{ "acos",  Eval::ACOS }
,	{ "acosh", Eval::ACOSH }
,	{ "asin",  Eval::ASIN }
,	{ "asinh", Eval::ASINH }
,	{ "atan",  Eval::ATAN }
,	{ "atanh", Eval::ATANH }
,	{ "ceil",  Eval::CEIL }
,	{ "cos",   Eval::COS }
,	{ "cosh",  Eval::COSH }
,	{ "erf",   Eval::ERF }
,	{ "erfc",  Eval::ERFC }
,	{ "exp",   Eval::EXP }
,	{ "exp10", Eval::EXP10 }
,	{ "exp2",  Eval::EXP2 }
,	{ "floor", Eval::FLOOR }
,	{ "log",   Eval::LOG }
,	{ "log10", Eval::LOG10 }
,	{ "log2",  Eval::LOG2 }
,	{ "sin",   Eval::SIN }
,	{ "sinh",  Eval::SINH }
,	{ "tan",   Eval::TAN }
,	{ "tanh",  Eval::TANH }
};

// MUST BE ORDERED!
const Parser::regEntry Parser::regMap[] =
{	{ "elem_num",       { 38, R_RDA  } }
,	{ "element_number", { 38, R_RDA  } }
,	{ "exp",            { 54, R_WRAB } }
,	{ "host_int",       { 38, R_WRAB } }
,	{ "interrupt",      { 38, R_WRAB } }
,	{ "irq",            { 38, R_WRAB } }
,	{ "log",            { 55, R_WRAB } }
,	{ "ms_flags",       { 42, R_RWA  } }
,	{ "ms_mask",        { 42, R_RWA  } }
,	{ "mutex",          { 51, R_RWAB } }
,	{ "mutex_acq",      { 51, R_RDAB } }
,	{ "mutex_acquire",  { 51, R_RDAB } }
,	{ "mutex_rel",      { 51, R_WRAB } }
,	{ "mutex_release",  { 51, R_WRAB } }
,	{ "nop",            { 39, R_RWAB } }
,	{ "qpu_num",        { 38, R_RDB  } }
,	{ "qpu_number",     { 38, R_RDB  } }
,	{ "quad_x",         { 41, R_RDA  } }
,	{ "quad_y",         { 41, R_RDB  } }
,	{ "r0",             { 32, R_WRAB } }
,	{ "r1",             { 33, R_WRAB } }
,	{ "r2",             { 34, R_WRAB } }
,	{ "r3",             { 35, R_WRAB } }
,	{ "r4",             { 36, R_WRAB } }
,	{ "r5",             { 37, R_AB   } }
,	{ "r5quad",         { 37, R_WRA  } }
,	{ "r5rep",          { 37, R_WRB  } }
,	{ "ra0",            { 0,  R_RWA  } }
,	{ "ra1",            { 1,  R_RWA  } }
,	{ "ra10",           { 10, R_RWA  } }
,	{ "ra11",           { 11, R_RWA  } }
,	{ "ra12",           { 12, R_RWA  } }
,	{ "ra13",           { 13, R_RWA  } }
,	{ "ra14",           { 14, R_RWA  } }
,	{ "ra15",           { 15, R_RWA  } }
,	{ "ra16",           { 16, R_RWA  } }
,	{ "ra17",           { 17, R_RWA  } }
,	{ "ra18",           { 18, R_RWA  } }
,	{ "ra19",           { 19, R_RWA  } }
,	{ "ra2",            { 2,  R_RWA  } }
,	{ "ra20",           { 20, R_RWA  } }
,	{ "ra21",           { 21, R_RWA  } }
,	{ "ra22",           { 22, R_RWA  } }
,	{ "ra23",           { 23, R_RWA  } }
,	{ "ra24",           { 24, R_RWA  } }
,	{ "ra25",           { 25, R_RWA  } }
,	{ "ra26",           { 26, R_RWA  } }
,	{ "ra27",           { 27, R_RWA  } }
,	{ "ra28",           { 28, R_RWA  } }
,	{ "ra29",           { 29, R_RWA  } }
,	{ "ra3",            { 3,  R_RWA  } }
,	{ "ra30",           { 30, R_RWA  } }
,	{ "ra31",           { 31, R_RWA  } }
,	{ "ra32",           { 32, R_RWA  } }
,	{ "ra33",           { 33, R_RWA  } }
,	{ "ra34",           { 34, R_RWA  } }
,	{ "ra35",           { 35, R_RWA  } }
,	{ "ra36",           { 36, R_RWA  } }
,	{ "ra37",           { 37, R_RWA  } }
,	{ "ra38",           { 38, R_RWA  } }
,	{ "ra39",           { 39, R_RWA  } }
,	{ "ra4",            { 4,  R_RWA  } }
,	{ "ra40",           { 40, R_RWA  } }
,	{ "ra41",           { 41, R_RWA  } }
,	{ "ra42",           { 42, R_RWA  } }
,	{ "ra43",           { 43, R_RWA  } }
,	{ "ra44",           { 44, R_RWA  } }
,	{ "ra45",           { 45, R_RWA  } }
,	{ "ra46",           { 46, R_RWA  } }
,	{ "ra47",           { 47, R_RWA  } }
,	{ "ra48",           { 48, R_RWA  } }
,	{ "ra49",           { 49, R_RWA  } }
,	{ "ra5",            { 5,  R_RWA  } }
,	{ "ra50",           { 50, R_RWA  } }
,	{ "ra51",           { 51, R_RWA  } }
,	{ "ra52",           { 52, R_RWA  } }
,	{ "ra53",           { 53, R_RWA  } }
,	{ "ra54",           { 54, R_RWA  } }
,	{ "ra55",           { 55, R_RWA  } }
,	{ "ra56",           { 56, R_RWA  } }
,	{ "ra57",           { 57, R_RWA  } }
,	{ "ra58",           { 58, R_RWA  } }
,	{ "ra59",           { 59, R_RWA  } }
,	{ "ra6",            { 6,  R_RWA  } }
,	{ "ra60",           { 60, R_RWA  } }
,	{ "ra61",           { 61, R_RWA  } }
,	{ "ra62",           { 62, R_RWA  } }
,	{ "ra63",           { 63, R_RWA  } }
,	{ "ra7",            { 7,  R_RWA  } }
,	{ "ra8",            { 8,  R_RWA  } }
,	{ "ra9",            { 9,  R_RWA  } }
,	{ "rb0",            { 0,  R_RWB  } }
,	{ "rb1",            { 1,  R_RWB  } }
,	{ "rb10",           { 10, R_RWB  } }
,	{ "rb11",           { 11, R_RWB  } }
,	{ "rb12",           { 12, R_RWB  } }
,	{ "rb13",           { 13, R_RWB  } }
,	{ "rb14",           { 14, R_RWB  } }
,	{ "rb15",           { 15, R_RWB  } }
,	{ "rb16",           { 16, R_RWB  } }
,	{ "rb17",           { 17, R_RWB  } }
,	{ "rb18",           { 18, R_RWB  } }
,	{ "rb19",           { 19, R_RWB  } }
,	{ "rb2",            { 2,  R_RWB  } }
,	{ "rb20",           { 20, R_RWB  } }
,	{ "rb21",           { 21, R_RWB  } }
,	{ "rb22",           { 22, R_RWB  } }
,	{ "rb23",           { 23, R_RWB  } }
,	{ "rb24",           { 24, R_RWB  } }
,	{ "rb25",           { 25, R_RWB  } }
,	{ "rb26",           { 26, R_RWB  } }
,	{ "rb27",           { 27, R_RWB  } }
,	{ "rb28",           { 28, R_RWB  } }
,	{ "rb29",           { 29, R_RWB  } }
,	{ "rb3",            { 3,  R_RWB  } }
,	{ "rb30",           { 30, R_RWB  } }
,	{ "rb31",           { 31, R_RWB  } }
,	{ "rb32",           { 32, R_RWB  } }
,	{ "rb33",           { 33, R_RWB  } }
,	{ "rb34",           { 34, R_RWB  } }
,	{ "rb35",           { 35, R_RWB  } }
,	{ "rb36",           { 36, R_RWB  } }
,	{ "rb37",           { 37, R_RWB  } }
,	{ "rb38",           { 38, R_RWB  } }
,	{ "rb39",           { 39, R_RWB  } }
,	{ "rb4",            { 4,  R_RWB  } }
,	{ "rb40",           { 40, R_RWB  } }
,	{ "rb41",           { 41, R_RWB  } }
,	{ "rb42",           { 42, R_RWB  } }
,	{ "rb43",           { 43, R_RWB  } }
,	{ "rb44",           { 44, R_RWB  } }
,	{ "rb45",           { 45, R_RWB  } }
,	{ "rb46",           { 46, R_RWB  } }
,	{ "rb47",           { 47, R_RWB  } }
,	{ "rb48",           { 48, R_RWB  } }
,	{ "rb49",           { 49, R_RWB  } }
,	{ "rb5",            { 5,  R_RWB  } }
,	{ "rb50",           { 50, R_RWB  } }
,	{ "rb51",           { 51, R_RWB  } }
,	{ "rb52",           { 52, R_RWB  } }
,	{ "rb53",           { 53, R_RWB  } }
,	{ "rb54",           { 54, R_RWB  } }
,	{ "rb55",           { 55, R_RWB  } }
,	{ "rb56",           { 56, R_RWB  } }
,	{ "rb57",           { 57, R_RWB  } }
,	{ "rb58",           { 58, R_RWB  } }
,	{ "rb59",           { 59, R_RWB  } }
,	{ "rb6",            { 6,  R_RWB  } }
,	{ "rb60",           { 60, R_RWB  } }
,	{ "rb61",           { 61, R_RWB  } }
,	{ "rb62",           { 62, R_RWB  } }
,	{ "rb63",           { 63, R_RWB  } }
,	{ "rb7",            { 7,  R_RWB  } }
,	{ "rb8",            { 8,  R_RWB  } }
,	{ "rb9",            { 9,  R_RWB  } }
,	{ "recip",          { 52, R_WRAB } }
,	{ "recipsqrt",      { 53, R_WRAB } }
,	{ "rev_flag",       { 42, R_RWB  } }
,	{ "sacq0",          { 0,  R_SACQ } }
,	{ "sacq1",          { 1,  R_SACQ } }
,	{ "sacq10",         { 10, R_SACQ } }
,	{ "sacq11",         { 11, R_SACQ } }
,	{ "sacq12",         { 12, R_SACQ } }
,	{ "sacq13",         { 13, R_SACQ } }
,	{ "sacq14",         { 14, R_SACQ } }
,	{ "sacq15",         { 15, R_SACQ } }
,	{ "sacq2",          { 2,  R_SACQ } }
,	{ "sacq3",          { 3,  R_SACQ } }
,	{ "sacq4",          { 4,  R_SACQ } }
,	{ "sacq5",          { 5,  R_SACQ } }
,	{ "sacq6",          { 6,  R_SACQ } }
,	{ "sacq7",          { 7,  R_SACQ } }
,	{ "sacq8",          { 8,  R_SACQ } }
,	{ "sacq9",          { 9,  R_SACQ } }
,	{ "sfu_exp",        { 54, R_WRAB } }
,	{ "sfu_log",        { 55, R_WRAB } }
,	{ "sfu_recip",      { 52, R_WRAB } }
,	{ "sfu_recipsqrt",  { 53, R_WRAB } }
,	{ "srel0",          { 0,  R_SREL } }
,	{ "srel1",          { 1,  R_SREL } }
,	{ "srel10",         { 10, R_SREL } }
,	{ "srel11",         { 11, R_SREL } }
,	{ "srel12",         { 12, R_SREL } }
,	{ "srel13",         { 13, R_SREL } }
,	{ "srel14",         { 14, R_SREL } }
,	{ "srel15",         { 15, R_SREL } }
,	{ "srel2",          { 2,  R_SREL } }
,	{ "srel3",          { 3,  R_SREL } }
,	{ "srel4",          { 4,  R_SREL } }
,	{ "srel5",          { 5,  R_SREL } }
,	{ "srel6",          { 6,  R_SREL } }
,	{ "srel7",          { 7,  R_SREL } }
,	{ "srel8",          { 8,  R_SREL } }
,	{ "srel9",          { 9,  R_SREL } }
,	{ "stencil",        { 43, R_WRAB } }
,	{ "t0b",            { 59, R_WRAB } }
,	{ "t0r",            { 58, R_WRAB } }
,	{ "t0s",            { 56, R_WRAB } }
,	{ "t0t",            { 57, R_WRAB } }
,	{ "t1b",            { 63, R_WRAB } }
,	{ "t1r",            { 62, R_WRAB } }
,	{ "t1s",            { 60, R_WRAB } }
,	{ "t1t",            { 61, R_WRAB } }
,	{ "tlb_alpha_mask", { 47, R_WRAB } }
,	{ "tlb_colour_all", { 46, R_WRAB } }
,	{ "tlb_colour_ms",  { 45, R_WRAB } }
,	{ "tlb_stencil",    { 43, R_WRAB } }
,	{ "tlb_z",          { 44, R_WRAB } }
,	{ "tlbam",          { 47, R_WRAB } }
,	{ "tlbc",           { 46, R_WRAB } }
,	{ "tlbm",           { 45, R_WRAB } }
,	{ "tlbz",           { 44, R_WRAB } }
,	{ "tmu0_b",         { 59, R_WRAB } }
,	{ "tmu0_r",         { 58, R_WRAB } }
,	{ "tmu0_s",         { 56, R_WRAB } }
,	{ "tmu0_t",         { 57, R_WRAB } }
,	{ "tmu1_b",         { 63, R_WRAB } }
,	{ "tmu1_r",         { 62, R_WRAB } }
,	{ "tmu1_s",         { 60, R_WRAB } }
,	{ "tmu1_t",         { 61, R_WRAB } }
,	{ "tmu_noswap",     { 36, R_WRAB } }
,	{ "tmurs",          { 36, R_WRAB } }
,	{ "unif",           { 32, R_RDAB } }
,	{ "unif_addr",      { 40, R_WRA  } }
,	{ "unif_addr_rel",  { 40, R_WRB  } }
,	{ "uniform_read",   { 32, R_RDAB } }
,	{ "vary",           { 35, R_RDAB } }
,	{ "varying_read",   { 35, R_RDAB } }
,	{ "vpm",            { 48, R_RWAB } }
,	{ "vpm_ld_addr",    { 50, R_WRA  } }
,	{ "vpm_ld_busy",    { 49, R_RDA  } }
,	{ "vpm_ld_wait",    { 50, R_RDA  } }
,	{ "vpm_read",       { 48, R_RDAB } }
,	{ "vpm_st_addr",    { 50, R_WRB  } }
,	{ "vpm_st_busy",    { 49, R_RDB  } }
,	{ "vpm_st_wait",    { 50, R_RDB  } }
,	{ "vpm_write",      { 48, R_WRAB } }
,	{ "vpmvcd_rd_setup",{ 49, R_WRA  } }
,	{ "vpmvcd_wr_setup",{ 49, R_WRB  } }
,	{ "vr_addr",        { 50, R_WRA  } }
,	{ "vr_busy",        { 49, R_RDA  } }
,	{ "vr_setup",       { 49, R_WRA  } }
,	{ "vr_wait",        { 50, R_RDA  } }
,	{ "vw_addr",        { 50, R_WRB  } }
,	{ "vw_busy",        { 49, R_RDB  } }
,	{ "vw_setup",       { 49, R_WRB  } }
,	{ "vw_wait",        { 50, R_RDB  } }
,	{ "x_coord",        { 41, R_RWA  } }
,	{ "x_pixel_coord",  { 41, R_RWA  } }
,	{ "y_coord",        { 41, R_RWB  } }
,	{ "y_pixel_coord",  { 41, R_RWB  } }
};

/// Map with opcode tokens, must be ordered.
const Parser::opEntry<8> Parser::opcodeMap[] =
{	{ "add",    &Parser::assembleADD, ::Inst::A_ADD }
,	{ "amov",   &Parser::assembleMOV, ~(int)(IC_ADD) }
,	{ "and",    &Parser::assembleADD, ::Inst::A_AND }
,	{ "anop",   &Parser::assembleADD, ::Inst::A_NOP|0x80 }
,	{ "asr",    &Parser::assembleADD, ::Inst::A_ASR }
,	{ "av8adds",&Parser::assembleADD, ::Inst::A_V8ADDS|0x80 }
,	{ "av8subs",&Parser::assembleADD, ::Inst::A_V8SUBS|0x80 }
,	{ "bkpt",   &Parser::assembleSIG, ::Inst::S_BREAK }
,	{ "bra",    &Parser::assembleBRANCH, false }
,	{ "break",  &Parser::assembleSIG, ::Inst::S_BREAK }
,	{ "brr",    &Parser::assembleBRANCH, true }
,	{ "clz",    &Parser::assembleADD, ::Inst::A_CLZ }
,	{ "fadd",   &Parser::assembleADD, ::Inst::A_FADD }
,	{ "fmax",   &Parser::assembleADD, ::Inst::A_FMAX }
,	{ "fmaxabs",&Parser::assembleADD, ::Inst::A_FMAXABS }
,	{ "fmin",   &Parser::assembleADD, ::Inst::A_FMIN }
,	{ "fminabs",&Parser::assembleADD, ::Inst::A_FMINABS }
,	{ "fmul",   &Parser::assembleMUL, ::Inst::M_FMUL }
,	{ "fsub",   &Parser::assembleADD, ::Inst::A_FSUB }
,	{ "ftoi",   &Parser::assembleADD, ::Inst::A_FTOI }
,	{ "itof",   &Parser::assembleADD, ::Inst::A_ITOF }
,	{ "ldaltlb",&Parser::assembleSIG, ::Inst::S_LOADAM }
,	{ "ldcend", &Parser::assembleSIG, ::Inst::S_LDCEND }
,	{ "ldcoend",&Parser::assembleSIG, ::Inst::S_LDCEND }
,	{ "ldcotlb",&Parser::assembleSIG, ::Inst::S_LOADC }
,	{ "ldcvtlb",&Parser::assembleSIG, ::Inst::S_LOADCV }
,	{ "ldi",    &Parser::assembleMOV, ::Inst::L_LDI }
,	{ "ldipes", &Parser::assembleMOV, ::Inst::L_PES }
,	{ "ldipeu", &Parser::assembleMOV, ::Inst::L_PEU }
,	{ "ldtmu0", &Parser::assembleSIG, ::Inst::S_LDTMU0 }
,	{ "ldtmu1", &Parser::assembleSIG, ::Inst::S_LDTMU1 }
,	{ "loadam", &Parser::assembleSIG, ::Inst::S_LOADAM }
,	{ "loadc",  &Parser::assembleSIG, ::Inst::S_LOADC }
,	{ "loadcv", &Parser::assembleSIG, ::Inst::S_LOADCV }
,	{ "lswitch",&Parser::assembleSIG, ::Inst::S_LTHRSW }
,	{ "lthrsw", &Parser::assembleSIG, ::Inst::S_LTHRSW }
,	{ "max",    &Parser::assembleADD, ::Inst::A_MAX }
,	{ "min",    &Parser::assembleADD, ::Inst::A_MIN }
,	{ "mmov",   &Parser::assembleMOV, ~(int)(IC_MUL) }
,	{ "mnop",   &Parser::assembleMUL, ::Inst::M_NOP|0x80 }
,	{ "mov",    &Parser::assembleMOV, ~(int)(IC_ADD|IC_MUL) }
,	{ "mul24",  &Parser::assembleMUL, ::Inst::M_MUL24 }
,	{ "mv8adds",&Parser::assembleMUL, ::Inst::M_V8ADDS|0x80 }
,	{ "mv8subs",&Parser::assembleMUL, ::Inst::M_V8SUBS|0x80 }
,	{ "nop",    &Parser::assembleADD, ::Inst::A_NOP } // alternative M_NOP
,	{ "not",    &Parser::assembleADD, ::Inst::A_NOT }
,	{ "or",     &Parser::assembleADD, ::Inst::A_OR }
,	{ "read",   &Parser::assembleREAD }
,	{ "ror",    &Parser::assembleADD, ::Inst::A_ROR }
,	{ "sacq",   &Parser::assembleMOV, ::Inst::L_SEMA|0x80 }
,	{ "sbdone", &Parser::assembleSIG, ::Inst::S_SBDONE }
,	{ "sbwait", &Parser::assembleSIG, ::Inst::S_SBWAIT }
,	{ "shl",    &Parser::assembleADD, ::Inst::A_SHL }
,	{ "shr",    &Parser::assembleADD, ::Inst::A_SHR }
,	{ "srel",   &Parser::assembleMOV, ::Inst::L_SEMA }
,	{ "sub",    &Parser::assembleADD, ::Inst::A_SUB }
,	{ "tend",   &Parser::assembleSIG, ::Inst::S_THREND }
,	{ "thrend", &Parser::assembleSIG, ::Inst::S_THREND }
,	{ "thrsw",  &Parser::assembleSIG, ::Inst::S_THRSW }
,	{ "tswitch",&Parser::assembleSIG, ::Inst::S_THRSW }
,	{ "unlscb", &Parser::assembleSIG, ::Inst::S_SBDONE }
,	{ "v8adds", &Parser::assembleMUL, ::Inst::M_V8ADDS } // alternative A_V8ADDS
,	{ "v8max",  &Parser::assembleMUL, ::Inst::M_V8MAX }
,	{ "v8min",  &Parser::assembleMUL, ::Inst::M_V8MIN }
,	{ "v8muld", &Parser::assembleMUL, ::Inst::M_V8MULD }
,	{ "v8subs", &Parser::assembleMUL, ::Inst::M_V8SUBS } // alternative A_V8SUBS
,	{ "waitscb",&Parser::assembleSIG, ::Inst::S_SBWAIT }
,	{ "xor",    &Parser::assembleADD, ::Inst::A_XOR }
};

const Parser::opExtEntry Parser::extMap[] =
{	{ "16a",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16a   }
,	{ "16aclamp",       IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16aSI |rPUp::PACK }
,	{ "16af",           IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16aF  }
,	{ "16ai",           IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16aI  }
,	{ "16as",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16aS  |rPUp::PACK }
,	{ "16asi",          IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16aSI }
,	{ "16b",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16b   }
,	{ "16bclamp",       IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16bSI |rPUp::PACK }
,	{ "16bf",           IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16bF  }
,	{ "16bi",           IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_16bI  }
,	{ "16bs",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16bS  |rPUp::PACK }
,	{ "16bsi",          IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_16bSI }
,	{ "32",             IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_32    } // NOP
,	{ "32clamp",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_32SI  |rPUp::PACK }
,	{ "32s",            IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_32SI  |rPUp::PACK }
,	{ "32si",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_32SI  |rPUp::PACK }
,	{ "8888",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcd |rPUp::PACK }
,	{ "8888i",          IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdI|rPUp::PACK }
,	{ "8888s",          IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "8888sf",         IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdSF|rPUp::PACK }
,	{ "8888si",         IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdSI|rPUp::PACK }
,	{ "8a",             IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8a    }
,	{ "8abcd",          IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcd |rPUp::PACK }
,	{ "8abcdi",         IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdI|rPUp::PACK }
,	{ "8abcds",         IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "8abcdsf",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdSF|rPUp::PACK }
,	{ "8abcdsi",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdSI|rPUp::PACK }
,	{ "8aclamp",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8aS   |rPUp::PACK }
,	{ "8af",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8aF   |rPUp::UNPACK}
,	{ "8ai",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8aI   }
,	{ "8as",            IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8aS   |rPUp::PACK }
,	{ "8asf",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8aSF  }
,	{ "8asi",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8aSI  }
,	{ "8b",             IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8b    }
,	{ "8bclamp",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8bS   |rPUp::PACK }
,	{ "8bf",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8bF   |rPUp::UNPACK}
,	{ "8bi",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8bI   }
,	{ "8bs",            IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8bS   |rPUp::PACK }
,	{ "8bsf",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8bSF  }
,	{ "8bsi",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8bSI  }
,	{ "8c",             IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8c    }
,	{ "8cclamp",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8cS   |rPUp::PACK }
,	{ "8cf",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8cF   |rPUp::UNPACK}
,	{ "8ci",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8cI   }
,	{ "8cs",            IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8cS   |rPUp::PACK }
,	{ "8csf",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8cSF  }
,	{ "8csi",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8cSI  }
,	{ "8d",             IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8d    }
,	{ "8dclamp",        IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8dS   |rPUp::PACK }
,	{ "8ddupe",         IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8abcd }
,	{ "8ddupeclamp",    IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "8df",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8dF   |rPUp::UNPACK}
,	{ "8di",            IC_XP|IC_SRC|IC_DST, &Parser::addPUp,  ::Inst::P_8dI   }
,	{ "8dr",            IC_XP|IC_SRC,        &Parser::addPUp,  ::Inst::U_8dr   |rPUp::UNPACK }
,	{ "8ds",            IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8dS   |rPUp::PACK }
,	{ "8dsf",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8dSF  }
,	{ "8dsi",           IC_XP|IC_DST,        &Parser::addPUp,  ::Inst::P_8dSI  }
,	{ "allc",           IC_OP,               &Parser::addCond, ::Inst::B_ALLCS }
,	{ "allcc",          IC_OP,               &Parser::addCond, ::Inst::B_ALLCC }
,	{ "allcs",          IC_OP,               &Parser::addCond, ::Inst::B_ALLCS }
,	{ "alln",           IC_OP,               &Parser::addCond, ::Inst::B_ALLNS }
,	{ "allnc",          IC_OP,               &Parser::addCond, ::Inst::B_ALLCC }
,	{ "allnn",          IC_OP,               &Parser::addCond, ::Inst::B_ALLNN }
,	{ "allnz",          IC_OP,               &Parser::addCond, ::Inst::B_ALLZC }
,	{ "allz",           IC_OP,               &Parser::addCond, ::Inst::B_ALLZS }
,	{ "anyc",           IC_OP,               &Parser::addCond, ::Inst::B_ANYCS }
,	{ "anycc",          IC_OP,               &Parser::addCond, ::Inst::B_ANYCC }
,	{ "anycs",          IC_OP,               &Parser::addCond, ::Inst::B_ANYCS }
,	{ "anyn",           IC_OP,               &Parser::addCond, ::Inst::B_ANYNS }
,	{ "anync",          IC_OP,               &Parser::addCond, ::Inst::B_ANYCC }
,	{ "anynn",          IC_OP,               &Parser::addCond, ::Inst::B_ANYNC }
,	{ "anynz",          IC_OP,               &Parser::addCond, ::Inst::B_ANYZC }
,	{ "anyz",           IC_OP,               &Parser::addCond, ::Inst::B_ANYZS }
,	{ "c",              IC_DST,              &Parser::addIf,   ::Inst::C_CS }
,	{ "cc",             IC_DST,              &Parser::addIf,   ::Inst::C_CC }
,	{ "cs",             IC_DST,              &Parser::addIf,   ::Inst::C_CS }
,	{ "ifc",            IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_CS }
,	{ "ifcc",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_CC }
,	{ "ifcs",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_CS }
,	{ "ifn",            IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_NS }
,	{ "ifnc",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_NC }
,	{ "ifnn",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_NC }
,	{ "ifns",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_NS }
,	{ "ifnz",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_ZC }
,	{ "ifz",            IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_ZS }
,	{ "ifzc",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_ZC }
,	{ "ifzs",           IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_ZS }
,	{ "n",              IC_DST,              &Parser::addIf,   ::Inst::C_NS }
,	{ "nc",             IC_DST,              &Parser::addIf,   ::Inst::C_NC }
,	{ "never",          IC_OP|IC_DST,        &Parser::addIf,   ::Inst::C_NEVER }
,	{ "nn",             IC_DST,              &Parser::addIf,   ::Inst::C_NC }
,	{ "ns",             IC_DST,              &Parser::addIf,   ::Inst::C_NS }
,	{ "nz",             IC_DST,              &Parser::addIf,   ::Inst::C_ZC }
,	{ "pack16a",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16a   |rPUp::PACK }
,	{ "pack16aclamp",   IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16aSI |rPUp::PACK }
,	{ "pack16af",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16aF  |rPUp::PACK }
,	{ "pack16ai",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16aI  |rPUp::PACK }
,	{ "pack16as",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16aS  |rPUp::PACK }
,	{ "pack16asi",      IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16aSI |rPUp::PACK }
,	{ "pack16b",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16b   |rPUp::PACK }
,	{ "pack16bclamp",   IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16bSI |rPUp::PACK }
,	{ "pack16bf",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16bF  |rPUp::PACK }
,	{ "pack16bi",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16bI  |rPUp::PACK }
,	{ "pack16bs",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16bI  |rPUp::PACK }
,	{ "pack16bsi",      IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_16bSI |rPUp::PACK }
,	{ "pack32",         IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_32    |rPUp::PACK } // NOP
,	{ "pack32clamp",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_32S   |rPUp::PACK }
,	{ "pack32s",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_32S   |rPUp::PACK }
,	{ "pack8888",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcd |rPUp::PACK }
,	{ "pack8888i",      IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdI|rPUp::PACK }
,	{ "pack8888s",      IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "pack8888sf",     IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdSF|rPUp::PACK }
,	{ "pack8888si",     IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdSI|rPUp::PACK }
,	{ "pack8a",         IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8a    |rPUp::PACK }
,	{ "pack8abcd",      IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcd |rPUp::PACK }
,	{ "pack8abcdi",     IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdI|rPUp::PACK }
,	{ "pack8abcds",     IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "pack8abcdsf",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdSF|rPUp::PACK }
,	{ "pack8abcdsi",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdSI|rPUp::PACK }
,	{ "pack8aclamp",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aS   |rPUp::PACK }
,	{ "pack8af",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aF   |rPUp::PACK }
,	{ "pack8ai",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aI   |rPUp::PACK }
,	{ "pack8as",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aS   |rPUp::PACK }
,	{ "pack8asf",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aSF  |rPUp::PACK }
,	{ "pack8asi",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8aSI  |rPUp::PACK }
,	{ "pack8b",         IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8b    |rPUp::PACK }
,	{ "pack8bclamp",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bS   |rPUp::PACK }
,	{ "pack8bf",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bF   |rPUp::PACK }
,	{ "pack8bi",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bI   |rPUp::PACK }
,	{ "pack8bs",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bS   |rPUp::PACK }
,	{ "pack8bsf",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bSF  |rPUp::PACK }
,	{ "pack8bsi",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8bSI  |rPUp::PACK }
,	{ "pack8c",         IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8c    |rPUp::PACK }
,	{ "pack8cclamp",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cS   |rPUp::PACK }
,	{ "pack8cf",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cF   |rPUp::PACK }
,	{ "pack8ci",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cI   |rPUp::PACK }
,	{ "pack8cs",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cS   |rPUp::PACK }
,	{ "pack8csf",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cSF  |rPUp::PACK }
,	{ "pack8csi",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8cSI  |rPUp::PACK }
,	{ "pack8d",         IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8d    |rPUp::PACK }
,	{ "pack8dclamp",    IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dS   |rPUp::PACK }
,	{ "pack8ddupe",     IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcd |rPUp::PACK }
,	{ "pack8ddupeclamp",IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8abcdS|rPUp::PACK }
,	{ "pack8df",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dF   |rPUp::PACK }
,	{ "pack8di",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dI   |rPUp::PACK }
,	{ "pack8ds",        IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dS   |rPUp::PACK }
,	{ "pack8dsf",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dSF  |rPUp::PACK }
,	{ "pack8dsi",       IC_XP|IC_OP|IC_DST,  &Parser::addPUp,  ::Inst::P_8dSI  |rPUp::PACK }
,	{ "rot",            IC_OP|IC_DST,        &Parser::addRot,  0 }
,	{ "setf",           IC_OP|IC_DST,        &Parser::addSetF, 0 }
,	{ "sf",             IC_OP|IC_DST,        &Parser::addSetF, 0 }
,	{ "unpack16a",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16a   |rPUp::UNPACK }
,	{ "unpack16af",     IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16aF  |rPUp::UNPACK }
,	{ "unpack16ai",     IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16aI  |rPUp::UNPACK }
,	{ "unpack16b",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16b   |rPUp::UNPACK }
,	{ "unpack16bf",     IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16bF  |rPUp::UNPACK }
,	{ "unpack16bi",     IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_16bI  |rPUp::UNPACK }
,	{ "unpack32",       IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_32    |rPUp::UNPACK } // NOP
,	{ "unpack8a",       IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8a    |rPUp::UNPACK }
,	{ "unpack8af",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8aF   |rPUp::UNPACK }
,	{ "unpack8ai",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8aI   |rPUp::UNPACK }
,	{ "unpack8b",       IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8b    |rPUp::UNPACK }
,	{ "unpack8bf",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8bF   |rPUp::UNPACK }
,	{ "unpack8bi",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8bI   |rPUp::UNPACK }
,	{ "unpack8c",       IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8c    |rPUp::UNPACK }
,	{ "unpack8cf",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8cF   |rPUp::UNPACK }
,	{ "unpack8ci",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8cI   |rPUp::UNPACK }
,	{ "unpack8d",       IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8d    |rPUp::UNPACK }
,	{ "unpack8ddupe",   IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8dr   |rPUp::UNPACK }
,	{ "unpack8df",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8dF   |rPUp::UNPACK }
,	{ "unpack8di",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8dI   |rPUp::UNPACK }
,	{ "unpack8dr",      IC_XP|IC_OP|IC_SRC,  &Parser::addPUp,  ::Inst::U_8dr   |rPUp::UNPACK }
,	{ "z",              IC_DST,              &Parser::addIf,   ::Inst::C_ZS }
,	{ "zc",             IC_DST,              &Parser::addIf,   ::Inst::C_ZC }
,	{ "zs",             IC_DST,              &Parser::addIf,   ::Inst::C_ZS }
};

const Parser::opEntry<8> Parser::directiveMap[] =
{	{ "align",   &Parser::parseALIGN, -1 }
,	{ "assert",  &Parser::parseASSERT }
,	{ "back",    &Parser::beginBACK }
,	{ "bit",     &Parser::parseDATA,  1 }
,	{ "byte",    &Parser::parseDATA,  8 }
,	{ "clone",   &Parser::parseCLONE }
,	{ "code",    &Parser::doSEGMENT,  SF_Code }
,	{ "const",   &Parser::parseSET,   C_CONST }
,	{ "define",  &Parser::parseSET,   C_NONE }
,	{ "double",  &Parser::parseDATA,  -64 }
,	{ "dword",   &Parser::parseDATA,  32 }
,	{ "elif",    &Parser::parseELSEIF }
,	{ "else",    &Parser::parseELSE }
,	{ "elseif",  &Parser::parseELSEIF }
,	{ "endb",    &Parser::endBACK }
,	{ "endback", &Parser::endBACK }
,	{ "endf",    &Parser::endMACRO,   M_FUNC }
,	{ "endfor",  &Parser::endREP,     1 }
,	{ "endfunc", &Parser::endMACRO,   M_FUNC }
,	{ "endif",   &Parser::parseENDIF }
,	{ "endloc",  &Parser::endLOCAL }
,	{ "endm",    &Parser::endMACRO,   M_NONE }
,	{ "endr",    &Parser::endREP,     0 }
,	{ "endrep",  &Parser::endREP,     0 }
,	{ "equ",     &Parser::parseSET,   C_NONE }
,	{ "float",   &Parser::parseDATA,  -32 }
,	{ "float16", &Parser::parseDATA,  -16 }
,	{ "float32", &Parser::parseDATA,  -32 }
,	{ "float64", &Parser::parseDATA,  -64 }
,	{ "foreach", &Parser::beginREP,   1 }
,	{ "func",    &Parser::beginMACRO, M_FUNC }
,	{ "global",  &Parser::parseGLOBAL }
,	{ "half",    &Parser::parseDATA,  -16 }
,	{ "if",      &Parser::parseIF }
,	{ "ifset",   &Parser::parseIFSET, C_NONE }
,	{ "include", &Parser::doINCLUDE }
,	{ "int",     &Parser::parseDATA,  32 }
,	{ "int1",    &Parser::parseDATA,  1 }
,	{ "int16",   &Parser::parseDATA,  16 }
,	{ "int2",    &Parser::parseDATA,  2 }
,	{ "int32",   &Parser::parseDATA,  32 }
,	{ "int4",    &Parser::parseDATA,  4 }
,	{ "int64",   &Parser::parseDATA,  64 }
,	{ "int8",    &Parser::parseDATA,  8 }
,	{ "lconst",  &Parser::parseSET,   C_LOCAL|C_CONST }
,	{ "local",   &Parser::beginLOCAL }
,	{ "long",    &Parser::parseDATA,  64 }
,	{ "lset",    &Parser::parseSET,   C_LOCAL }
,	{ "lunset",  &Parser::parseUNSET, C_LOCAL }
,	{ "macro",   &Parser::beginMACRO, M_NONE }
,	{ "qword",   &Parser::parseDATA,  64 }
,	{ "rep",     &Parser::beginREP,   0 }
,	{ "rodata",  &Parser::doSEGMENT,  SF_Data }
,	{ "set",     &Parser::parseSET,   C_NONE }
,	{ "short",   &Parser::parseDATA,  16 }
,	{ "text",    &Parser::doSEGMENT,  SF_Code }
,	{ "undef",   &Parser::parseUNSET, C_NONE }
,	{ "unset",   &Parser::parseUNSET, C_NONE }
,	{ "word",    &Parser::parseDATA,  16 }
};
