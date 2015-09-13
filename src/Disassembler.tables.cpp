/*
 * Disassembler.tables.cpp
 *
 *  Created on: 12.11.2014
 *      Author: mueller
 */

const char Disassembler::cRreg[2][64][10] = {
{	"ra0", "ra1", "ra2", "ra3", "ra4", "ra5", "ra6", "ra7",
	"ra8", "ra9", "ra10", "ra11", "ra12", "ra13", "ra14", "ra15", //ra15 is w in shaders
	"ra16", "ra17", "ra18", "ra19", "ra20", "ra21", "ra22", "ra23",
	"ra24", "ra25", "ra26", "ra27", "ra28", "ra29", "ra30", "ra31",
	"unif", "ra33?", "ra34?", "vary", "ra36?", "ra37?", "elem_num", "nop",
	"ra40", "x_coord", "ms_mask", "ra43?", "ra44?", "ra45?", "ra46?", "ra47?",
	"vpm", "vr_busy", "vr_wait", "mutex", "ra52?", "ra53?", "ra54?", "ra55?",
	"ra56?", "ra57?", "ra58?", "ra59?", "ra60?", "ra61?", "ra62?", "ra63?",
},
{	"rb0", "rb1", "rb2", "rb3", "rb4", "rb5", "rb6", "rb7",
	"rb8", "rb9", "rb10", "rb11", "rb12", "rb13", "rb14", "rb15", //rb15 is z in shaders
	"rb16", "rb17", "rb18", "rb19", "rb20", "rb21", "rb22", "rb23",
	"rb24", "rb25", "rb26", "rb27", "rb28", "rb29", "rb30", "rb31",
	"unif", "rb33?", "rb34?", "vary", "rb36?", "rb37?", "qpu_num", "nop",
	"rb40?", "y_coord", "rev_flag", "rb43?", "rb44?", "rb45?", "rb46?", "rb47?",
	"vpm", "vw_busy", "vw_wait", "mutex", "rb52?", "rb53?", "rb54?", "rb55?",
	"rb56?", "rb57?", "rb58?", "rb59?", "rb60?", "rb61?", "rb62?", "rb63?",
} };
const char Disassembler::cWreg[2][64][14] = {
{	"ra0", "ra1", "ra2", "ra3", "ra4", "ra5", "ra6", "ra7",
	"ra8", "ra9", "ra10", "ra11", "ra12", "ra13", "ra14", "ra15", //ra15 is w in shaders
	"ra16", "ra17", "ra18", "ra19", "ra20", "ra21", "ra22", "ra23",
	"ra24", "ra25", "ra26", "ra27", "ra28", "ra29", "ra30", "ra31",
	"r0", "r1", "r2", "r3", "tmurs", "r5quad", "irq", "-",
	"unif_addr", "x_coord", "ms_mask", "stencil", "tlbz", "tlbm", "tlbc", "tlbam",
	"vpm", "vr_setup", "vr_addr", "mutex", "recip", "recipsqrt", "exp", "log",
	"t0s", "t0t", "t0r", "t0b", "t1s", "t1t", "t1r", "t1b",
},
{	"rb0", "rb1", "rb2", "rb3", "rb4", "rb5", "rb6", "rb7",
	"rb8", "rb9", "rb10", "rb11", "rb12", "rb13", "rb14", "rb15", //rb15 is z in shaders
	"rb16", "rb17", "rb18", "rb19", "rb20", "rb21", "rb22", "rb23",
	"rb24", "rb25", "rb26", "rb27", "rb28", "rb29", "rb30", "rb31",
	"r0", "r1", "r2", "r3", "tmurs", "r5rep", "irq", "-",
	"unif_addr_rel", "y_coord", "rev_flag", "stencil", "tlbz", "tlbm", "tlbc", "tlbam",
	"vpm", "vw_setup", "vw_addr", "mutex", "recip", "recipsqrt", "exp", "log",
	"t0s", "t0t", "t0r", "t0b", "t1s", "t1t", "t1r", "t1b",
} };

const char Disassembler::cOpA[33][9] =
{	"nop", "fadd", "fsub", "fmin", "fmax", "fminabs", "fmaxabs", "ftoi",
	"itof", "addop9?", "addop10?", "addop11?", "add", "sub", "shr", "asr",
	"ror", "shl", "min", "max", "and", "or", "xor", "not",
	"clz", "addop25?", "addop26?", "addop27?", "addop28?", "addop29?", "v8adds", "v8subs",
	"mov"
};

const char Disassembler::cOpM[9][7] =
{	"nop", "fmul", "mul24", "v8muld", "v8min", "v8max", "v8adds", "v8subs",
	"mov"
};

const char Disassembler::cOpL[8][7] =
{ "ldi", "ldipes", "ldi02?", "ldipeu", "s", "s01?", "s02?", "s03?"
};

const char Disassembler::cOpS[14][10] =
{	";  bkpt", "", ";  thrsw", ";  thrend", ";  sbwait", ";  sbdone", ";  lthrsw", ";  loadcv",
	";  loadc", ";  ldcend", ";  ldtmu0", ";  ldtmu1", ";  loadam", ""
};

const char Disassembler::cCC[8][7] =
{	".never", "", ".ifz", ".ifnz", ".ifn", ".ifnn", ".ifc", ".ifcc"
};

const char Disassembler::cPack[2][16][8] = {
{	"", ".16a", ".16b", ".8abcd", ".8a", ".8b", ".8c", ".8d",
	".32s", ".16as", ".16bs", ".8abcds", ".8as", ".8bs", ".8cs", ".8ds"
},
{	"", ".pm01?", ".pm02?", ".8abcds", ".8as", ".8bs", ".8cs", ".8ds",
	".pm08?", ".pm09?", ".pm10?", ".pm11?", ".pm12?", ".pm13?", ".pm14?", ".pm15?"
} };

const char Disassembler::cUnpack[8][5] =
{	"", ".16a", ".16b", ".8dr", ".8a", ".8b", ".8c", ".8d"
};

const char Disassembler::cBCC[16][7] =
{	".allz", ".allnz", ".anyz", ".anynz", ".alln", ".allnn", ".anyn", ".anynn",
	".allc", ".allnc", ".anyc", ".anync", ".cc12?", ".cc13?", ".cc14?", ""
};

const char Disassembler::cSMI[64][7] =
{	"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15",
	"-16", "-15", "-14", "-13", "-12", "-11", "-10", "-9",
	"-8", "-7", "-6", "-5", "-4", "-3", "-2", "-1",
	"1.0", "2.0", "4.0", "8.0", "16.0", "32.0", "64.0", "128.0",
	"1./256", "1./128", "1./64", "1./32", "1./16", "1./8", "1./4", "1./2",
	" >> r5", " >> 1", " >> 2", " >> 3", " >> 4", " >> 5", " >> 6", " >> 7",
	" >> 8", " << 7", " << 6", " << 5", " << 4", " << 3", " << 2", " << 1"
};
