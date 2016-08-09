/*
 * AssembleInst.tables.cpp
 *
 *  Created on: 19.06.2016
 *      Author: mueller
 */

// Ordered by value and priority descending
const AssembleInst::smiEntry AssembleInst::smiMap[] =
{	{ 0x00000000,  0, A_XOR }
,	{ 0x00000000,  0, M_V8SUBS }
,	{ 0x00000001,  1, A_OR }
,	{ 0x00000001, 32, A_FTOI }
,	{ 0x00000001, 30, A_NOT }
,	{ 0x00000001, 31, A_SHR }
,	{ 0x00000001,  1, M_V8MIN }
,	{ 0x00000001, 12, M_V8MULD }
,	{ 0x00000001, 13, M_V8MULD }
,	{ 0x00000001, 14, M_V8MULD }
,	{ 0x00000001, 15, M_V8MULD }
,	{ 0x00000001, 33, A_CLZ }
,	{ 0x00000001, 34, A_CLZ }
,	{ 0x00000001, 35, A_CLZ }
,	{ 0x00000001, 36, A_CLZ }
,	{ 0x00000001, 37, A_CLZ }
,	{ 0x00000001, 38, A_CLZ }
,	{ 0x00000001, 39, A_CLZ }
,	{ 0x00000001, 62, A_NOT }
,	{ 0x00000001, 63, A_SHR }
,	{ 0x00000002,  2, A_OR }
,	{ 0x00000002, 33, A_FTOI }
,	{ 0x00000002,  1, A_ADD }
,	{ 0x00000002, 29, A_NOT }
,	{ 0x00000002,  2, M_V8MIN }
,	{ 0x00000002,  1, M_V8ADDS }
,	{ 0x00000002, 32, A_CLZ }
,	{ 0x00000002, 40, A_CLZ }
,	{ 0x00000002, 41, A_CLZ }
,	{ 0x00000002, 42, A_CLZ }
,	{ 0x00000002, 43, A_CLZ }
,	{ 0x00000002, 44, A_CLZ }
,	{ 0x00000002, 45, A_CLZ }
,	{ 0x00000002, 46, A_CLZ }
,	{ 0x00000002, 47, A_CLZ }
,	{ 0x00000002, 61, A_NOT }
,	{ 0x00000003,  3, A_OR }
,	{ 0x00000003, 28, A_NOT }
,	{ 0x00000003, 30, A_SHR }
,	{ 0x00000003,  3, M_V8MIN }
,	{ 0x00000003, 60, A_NOT }
,	{ 0x00000003, 62, A_SHR }
,	{ 0x00000004,  4, A_OR }
,	{ 0x00000004,  2, A_ADD }
,	{ 0x00000004, 34, A_FTOI }
,	{ 0x00000004, 27, A_NOT }
,	{ 0x00000004,  4, M_V8MIN }
,	{ 0x00000004,  2, M_MUL24 }
,	{ 0x00000004, 59, A_NOT }
,	{ 0x00000005,  5, A_OR }
,	{ 0x00000005, 26, A_NOT }
,	{ 0x00000005,  5, M_V8MIN }
,	{ 0x00000005, 58, A_NOT }
,	{ 0x00000006,  6, A_OR }
,	{ 0x00000006,  3, A_ADD }
,	{ 0x00000006, 25, A_NOT }
,	{ 0x00000006,  6, M_V8MIN }
,	{ 0x00000006,  3, M_V8ADDS }
,	{ 0x00000006, 57, A_NOT }
,	{ 0x00000007,  7, A_OR }
,	{ 0x00000007, 24, A_NOT }
,	{ 0x00000007, 29, A_SHR }
,	{ 0x00000007,  7, M_V8MIN }
,	{ 0x00000007, 56, A_NOT }
,	{ 0x00000007, 61, A_SHR }
,	{ 0x00000008,  8, A_OR }
,	{ 0x00000008,  4, A_ADD }
,	{ 0x00000008,  2, A_SHL }
,	{ 0x00000008, 35, A_FTOI }
,	{ 0x00000008, 23, A_NOT }
,	{ 0x00000008,  8, M_V8MIN }
,	{ 0x00000008,  4, M_V8ADDS }
,	{ 0x00000008, 55, A_NOT }
,	{ 0x00000009,  9, A_OR }
,	{ 0x00000009, 22, A_NOT }
,	{ 0x00000009,  9, M_V8MIN }
,	{ 0x00000009,  3, M_MUL24 }
,	{ 0x00000009, 54, A_NOT }
,	{ 0x0000000a, 10, A_OR }
,	{ 0x0000000a,  5, A_ADD }
,	{ 0x0000000a, 21, A_NOT }
,	{ 0x0000000a, 10, M_V8MIN }
,	{ 0x0000000a,  5, M_V8ADDS }
,	{ 0x0000000a, 53, A_NOT }
,	{ 0x0000000b, 11, A_OR }
,	{ 0x0000000b, 20, A_NOT }
,	{ 0x0000000b, 11, M_V8MIN }
,	{ 0x0000000b, 52, A_NOT }
,	{ 0x0000000c, 12, A_OR }
,	{ 0x0000000c,  6, A_ADD }
,	{ 0x0000000c, 19, A_NOT }
,	{ 0x0000000c, 12, M_V8MIN }
,	{ 0x0000000c,  6, M_V8ADDS }
,	{ 0x0000000c, 51, A_NOT }
,	{ 0x0000000d, 13, A_OR }
,	{ 0x0000000d, 18, A_NOT }
,	{ 0x0000000d, 13, M_V8MIN }
,	{ 0x0000000d, 50, A_NOT }
,	{ 0x0000000e, 14, A_OR }
,	{ 0x0000000e,  7, A_ADD }
,	{ 0x0000000e, 17, A_NOT }
,	{ 0x0000000e, 14, M_V8MIN }
,	{ 0x0000000e,  7, M_V8ADDS }
,	{ 0x0000000e, 49, A_NOT }
,	{ 0x0000000f, 15, A_OR }  // 15
,	{ 0x0000000f, 16, A_NOT }
,	{ 0x0000000f, 28, A_SHR }
,	{ 0x0000000f, 15, M_V8MIN }
,	{ 0x0000000f, 48, A_NOT }
,	{ 0x0000000f, 60, A_SHR }
,	{ 0x00000010,  8, A_ADD } // 16
,	{ 0x00000010,  4, M_MUL24 }
,	{ 0x00000010, 36, A_FTOI }
,	{ 0x00000010,  8, M_V8ADDS }
,	{ 0x00000012,  9, A_ADD } // 18
,	{ 0x00000012,  9, M_V8ADDS }
,	{ 0x00000014, 10, A_ADD } // 20
,	{ 0x00000014, 10, M_V8ADDS }
,	{ 0x00000016, 11, A_ADD } // 22
,	{ 0x00000016, 11, M_V8ADDS }
,	{ 0x00000018, 12, A_ADD } // 24
,	{ 0x00000018,  3, A_SHL }
,	{ 0x00000018, 12, M_V8ADDS }
,	{ 0x00000019,  5, M_MUL24 } // 25
,	{ 0x0000001a, 13, A_ADD } // 26
,	{ 0x0000001a, 13, M_V8ADDS }
,	{ 0x0000001c, 14, A_ADD } // 28
,	{ 0x0000001c, 14, M_V8ADDS }
,	{ 0x0000001c,  8, A_CLZ }
,	{ 0x0000001c,  9, A_CLZ }
,	{ 0x0000001c, 10, A_CLZ }
,	{ 0x0000001c, 11, A_CLZ }
,	{ 0x0000001c, 12, A_CLZ }
,	{ 0x0000001c, 13, A_CLZ }
,	{ 0x0000001c, 15, A_CLZ }
,	{ 0x0000001d,  4, A_CLZ } // 29
,	{ 0x0000001d,  5, A_CLZ }
,	{ 0x0000001d,  6, A_CLZ }
,	{ 0x0000001d,  7, A_CLZ }
,	{ 0x0000001e, 15, A_ADD } // 30
,	{ 0x0000001e,  2, A_CLZ }
,	{ 0x0000001e,  3, A_CLZ }
,	{ 0x0000001e, 15, M_V8ADDS }
,	{ 0x0000001f,  1, A_CLZ } // 31
,	{ 0x0000001f, 27, A_SHR }
,	{ 0x0000001f, 59, A_SHR }
,	{ 0x00000020, 37, A_FTOI } // 32
,	{ 0x00000020,  0, A_CLZ }
,	{ 0x00000024,  6, M_MUL24 } // 36
,	{ 0x00000031,  7, M_MUL24 } // 49
,	{ 0x0000003f, 26, A_SHR }   // 63
,	{ 0x0000003f, 58, A_SHR }
,	{ 0x00000040, 38, A_FTOI }  // 64
,	{ 0x00000040,  4, A_SHL }
,	{ 0x00000040,  8, M_MUL24 }
,	{ 0x00000051,  9, M_MUL24 } // 81
,	{ 0x00000064, 10, M_MUL24 } // 100
,	{ 0x00000079, 11, M_MUL24 } // 121
,	{ 0x0000007f, 25, A_SHR }   // 127
,	{ 0x0000007f, 57, A_SHR }
,	{ 0x00000080, 39, A_FTOI }  // 128
,	{ 0x00000090, 12, M_MUL24 } // 144
,	{ 0x000000a0,  5, A_SHL }   // 160
,	{ 0x000000a9, 13, M_MUL24 } // 169
,	{ 0x000000c4, 14, M_MUL24 } // 196
,	{ 0x000000e1, 15, M_MUL24 } // 225
,	{ 0x000000ff, 24, A_SHR } // 255
,	{ 0x000000ff, 56, A_SHR }
,	{ 0x00000180,  6, A_SHL } // 384
,	{ 0x000001ff, 23, A_SHR } // 511
,	{ 0x000001ff, 55, A_SHR }
,	{ 0x00000380,  7, A_SHL } // 896
,	{ 0x000003ff, 22, A_SHR } // 1023
,	{ 0x000003ff, 54, A_SHR }
,	{ 0x000007ff, 21, A_SHR } // 2047
,	{ 0x000007ff, 53, A_SHR }
,	{ 0x00000800,  8, A_SHL } // 2048
,	{ 0x00000fff, 20, A_SHR } // 4095
,	{ 0x00000fff, 52, A_SHR }
,	{ 0x00001200,  9, A_SHL } // 4608
,	{ 0x00001fff, 19, A_SHR } // 8191
,	{ 0x00001fff, 51, A_SHR }
,	{ 0x00002800, 10, A_SHL } // 10240
,	{ 0x00003fff, 18, A_SHR } // 16383
,	{ 0x00003fff, 50, A_SHR }
,	{ 0x00005800, 11, A_SHL } // 22528
,	{ 0x00007fff, 17, A_SHR } // 32767
,	{ 0x00007fff, 49, A_SHR }
,	{ 0x0000c000, 12, A_SHL } // 49152
,	{ 0x0000ffff, 16, A_SHR } // 65535
,	{ 0x0000ffff, 48, A_SHR }
,	{ 0x0001a000, 13, A_SHL } // 106496
,	{ 0x00038000, 14, A_SHL } // 229376
,	{ 0x00078000, 15, A_SHL } // 491520
,	{ 0x001e0000, 15, A_ROR }
,	{ 0x00380000, 14, A_ROR }
,	{ 0x00680000, 13, A_ROR }
,	{ 0x00c00000, 12, A_ROR }
,	{ 0x01010101,  1, A_OR,    P_8abcdI }
,	{ 0x01010101, 30, A_NOT,   P_8abcdI }
,	{ 0x01010101, 31, A_SHR,   P_8abcdI }
,	{ 0x01010101,  1, M_V8MIN, P_8abcdI }
,	{ 0x01010101, 31, M_MUL24, P_8abcdI }
,	{ 0x01010101, 12, M_V8MULD,P_8abcdI }
,	{ 0x01010101, 13, M_V8MULD,P_8abcdI }
,	{ 0x01010101, 14, M_V8MULD,P_8abcdI }
,	{ 0x01010101, 15, M_V8MULD,P_8abcdI }
,	{ 0x01010101, 32, A_FTOI,  P_8abcdI }
,	{ 0x01010101, 33, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 34, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 35, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 36, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 37, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 38, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 39, A_CLZ,   P_8abcdI }
,	{ 0x01010101, 40, M_V8MIN, P_8abcdSF }
,	{ 0x01010101, 44, M_FMUL,  P_8abcdSF }
,	{ 0x01010101,  1, A_OR,    P_8abcdSI }
,	{ 0x01010101, 30, A_NOT,   P_8abcdSI }
,	{ 0x01010101, 31, A_SHR,   P_8abcdSI }
,	{ 0x01010101,  1, M_V8MIN, P_8abcdSI }
,	{ 0x01010101, 12, M_V8MULD,P_8abcdSI }
,	{ 0x01010101, 13, M_V8MULD,P_8abcdSI }
,	{ 0x01010101, 14, M_V8MULD,P_8abcdSI }
,	{ 0x01010101, 15, M_V8MULD,P_8abcdSI }
,	{ 0x01010101, 32, A_FTOI,  P_8abcdSI }
,	{ 0x01010101, 33, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 34, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 35, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 36, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 37, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 38, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 39, A_CLZ,   P_8abcdSI }
,	{ 0x01010101, 62, A_NOT,   P_8abcdI }
,	{ 0x01010101, 63, A_SHR,   P_8abcdI }
,	{ 0x01010101, 62, A_NOT,   P_8abcdSI }
,	{ 0x01010101, 63, A_SHR,   P_8abcdSI }
,	{ 0x01600000, 11, A_ROR }
,	{ 0x02020202,  2, A_OR,    P_8abcdI }
,	{ 0x02020202, 33, A_FTOI,  P_8abcdI }
,	{ 0x02020202,  1, A_ADD,   P_8abcdI }
,	{ 0x02020202, 29, A_NOT,   P_8abcdI }
,	{ 0x02020202,  2, M_V8MIN, P_8abcdI }
,	{ 0x02020202,  1, M_V8ADDS,P_8abcdI }
,	{ 0x02020202, 32, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 40, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 41, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 42, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 43, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 44, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 45, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 46, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 47, A_CLZ,   P_8abcdI }
,	{ 0x02020202, 41, M_V8MIN, P_8abcdSF }
,	{ 0x02020202,  2, A_OR,    P_8abcdSI }
,	{ 0x02020202, 33, A_FTOI,  P_8abcdSI }
,	{ 0x02020202,  1, A_ADD,   P_8abcdSI }
,	{ 0x02020202, 29, A_NOT,   P_8abcdSI }
,	{ 0x02020202,  2, M_V8MIN, P_8abcdSI }
,	{ 0x02020202,  1, M_V8ADDS,P_8abcdSI }
,	{ 0x02020202, 32, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 40, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 41, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 42, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 43, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 44, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 45, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 46, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 47, A_CLZ,   P_8abcdSI }
,	{ 0x02020202, 61, A_NOT,   P_8abcdI }
,	{ 0x02020202, 61, A_NOT,   P_8abcdSI }
,	{ 0x02800000, 10, A_ROR }
,	{ 0x03030303,  3, A_OR,    P_8abcdI }
,	{ 0x03030303, 28, A_NOT,   P_8abcdI }
,	{ 0x03030303, 30, A_SHR,   P_8abcdI }
,	{ 0x03030303,  3, M_V8MIN, P_8abcdI }
,	{ 0x03030303,  3, A_OR,    P_8abcdSI }
,	{ 0x03030303, 28, A_NOT,   P_8abcdSI }
,	{ 0x03030303, 30, A_SHR,   P_8abcdSI }
,	{ 0x03030303,  3, M_V8MIN, P_8abcdSI }
,	{ 0x03030303, 60, A_NOT,   P_8abcdI }
,	{ 0x03030303, 62, A_SHR,   P_8abcdI }
,	{ 0x03030303, 60, A_NOT,   P_8abcdSI }
,	{ 0x03030303, 62, A_SHR,   P_8abcdSI }
,	{ 0x04040404,  4, A_OR,    P_8abcdI }
,	{ 0x04040404,  2, A_ADD,   P_8abcdI }
,	{ 0x04040404, 34, A_FTOI,  P_8abcdI }
,	{ 0x04040404, 27, A_NOT,   P_8abcdI }
,	{ 0x04040404,  4, M_V8MIN, P_8abcdI }
,	{ 0x04040404,  2, M_MUL24, P_8abcdI }
,	{ 0x04040404, 30, M_MUL24, P_8abcdI }
,	{ 0x04040404, 42, M_V8MIN, P_8abcdSF }
,	{ 0x04040404, 45, M_FMUL,  P_8abcdSF }
,	{ 0x04040404,  4, A_OR,    P_8abcdSI }
,	{ 0x04040404,  2, A_ADD,   P_8abcdSI }
,	{ 0x04040404, 34, A_FTOI,  P_8abcdSI }
,	{ 0x04040404, 27, A_NOT,   P_8abcdSI }
,	{ 0x04040404,  4, M_V8MIN, P_8abcdSI }
,	{ 0x04040404,  2, M_MUL24, P_8abcdSI }
,	{ 0x04040404, 59, A_NOT,   P_8abcdI }
,	{ 0x04040404, 59, A_NOT,   P_8abcdSI }
,	{ 0x04800000,  9, A_ROR }
,	{ 0x05050505,  5, A_OR,    P_8abcdI }
,	{ 0x05050505, 26, A_NOT,   P_8abcdI }
,	{ 0x05050505,  5, M_V8MIN, P_8abcdI }
,	{ 0x05050505,  5, A_OR,    P_8abcdSI }
,	{ 0x05050505, 26, A_NOT,   P_8abcdSI }
,	{ 0x05050505,  5, M_V8MIN, P_8abcdSI }
,	{ 0x05050505, 58, A_NOT,   P_8abcdI }
,	{ 0x05050505, 58, A_NOT,   P_8abcdSI }
,	{ 0x06060606,  6, A_OR,    P_8abcdI }
,	{ 0x06060606,  3, A_ADD,   P_8abcdI }
,	{ 0x06060606, 25, A_NOT,   P_8abcdI }
,	{ 0x06060606,  6, M_V8MIN, P_8abcdI }
,	{ 0x06060606,  3, M_V8ADDS,P_8abcdI }
,	{ 0x06060606,  6, A_OR,    P_8abcdSI }
,	{ 0x06060606,  3, A_ADD,   P_8abcdSI }
,	{ 0x06060606, 25, A_NOT,   P_8abcdSI }
,	{ 0x06060606,  6, M_V8MIN, P_8abcdSI }
,	{ 0x06060606,  3, M_V8ADDS,P_8abcdSI }
,	{ 0x06060606, 57, A_NOT,   P_8abcdI }
,	{ 0x06060606, 57, A_NOT,   P_8abcdSI }
,	{ 0x07070707,  7, A_OR,    P_8abcdI }
,	{ 0x07070707, 24, A_NOT,   P_8abcdI }
,	{ 0x07070707, 29, A_SHR,   P_8abcdI }
,	{ 0x07070707,  7, M_V8MIN, P_8abcdI }
,	{ 0x07070707,  7, A_OR,    P_8abcdSI }
,	{ 0x07070707, 24, A_NOT,   P_8abcdSI }
,	{ 0x07070707, 29, A_SHR,   P_8abcdSI }
,	{ 0x07070707,  7, M_V8MIN, P_8abcdSI }
,	{ 0x07070707, 56, A_NOT,   P_8abcdI }
,	{ 0x07070707, 61, A_SHR,   P_8abcdI }
,	{ 0x07070707, 56, A_NOT,   P_8abcdSI }
,	{ 0x07070707, 61, A_SHR,   P_8abcdSI }
,	{ 0x08000000,  8, A_ROR }
,	{ 0x08080808,  8, A_OR,    P_8abcdI }
,	{ 0x08080808,  4, A_ADD,   P_8abcdI }
,	{ 0x08080808,  2, A_SHL,   P_8abcdI }
,	{ 0x08080808, 35, A_FTOI,  P_8abcdI }
,	{ 0x08080808, 23, A_NOT,   P_8abcdI }
,	{ 0x08080808,  8, M_V8MIN, P_8abcdI }
,	{ 0x08080808,  4, M_V8ADDS,P_8abcdI }
,	{ 0x08080808, 43, M_V8MIN, P_8abcdSF }
,	{ 0x08080808,  8, A_OR,    P_8abcdSI }
,	{ 0x08080808,  4, A_ADD,   P_8abcdSI }
,	{ 0x08080808,  2, A_SHL,   P_8abcdSI }
,	{ 0x08080808, 35, A_FTOI,  P_8abcdSI }
,	{ 0x08080808, 23, A_NOT,   P_8abcdSI }
,	{ 0x08080808,  8, M_V8MIN, P_8abcdSI }
,	{ 0x08080808,  4, M_V8ADDS,P_8abcdSI }
,	{ 0x08080808, 55, A_NOT,   P_8abcdI }
,	{ 0x08080808, 55, A_NOT,   P_8abcdSI }
,	{ 0x09090909,  9, A_OR,    P_8abcdI }
,	{ 0x09090909, 22, A_NOT,   P_8abcdI }
,	{ 0x09090909,  9, M_V8MIN, P_8abcdI }
,	{ 0x09090909,  3, M_MUL24, P_8abcdI }
,	{ 0x09090909, 29, M_MUL24, P_8abcdI }
,	{ 0x09090909,  9, A_OR,    P_8abcdSI }
,	{ 0x09090909, 22, A_NOT,   P_8abcdSI }
,	{ 0x09090909,  9, M_V8MIN, P_8abcdSI }
,	{ 0x09090909,  3, M_MUL24, P_8abcdSI }
,	{ 0x09090909, 54, A_NOT,   P_8abcdI }
,	{ 0x09090909, 54, A_NOT,   P_8abcdSI }
,	{ 0x0a0a0a0a, 10, A_OR,    P_8abcdI }
,	{ 0x0a0a0a0a,  5, A_ADD,   P_8abcdI }
,	{ 0x0a0a0a0a, 21, A_NOT,   P_8abcdI }
,	{ 0x0a0a0a0a, 10, M_V8MIN, P_8abcdI }
,	{ 0x0a0a0a0a,  5, M_V8ADDS,P_8abcdI }
,	{ 0x0a0a0a0a, 10, A_OR,    P_8abcdSI }
,	{ 0x0a0a0a0a,  5, A_ADD,   P_8abcdSI }
,	{ 0x0a0a0a0a, 21, A_NOT,   P_8abcdSI }
,	{ 0x0a0a0a0a, 10, M_V8MIN, P_8abcdSI }
,	{ 0x0a0a0a0a,  5, M_V8ADDS,P_8abcdSI }
,	{ 0x0a0a0a0a, 53, A_NOT,   P_8abcdI }
,	{ 0x0a0a0a0a, 53, A_NOT,   P_8abcdSI }
,	{ 0x0b0b0b0b, 11, A_OR,    P_8abcdI }
,	{ 0x0b0b0b0b, 20, A_NOT,   P_8abcdI }
,	{ 0x0b0b0b0b, 11, M_V8MIN, P_8abcdI }
,	{ 0x0b0b0b0b, 11, A_OR,    P_8abcdSI }
,	{ 0x0b0b0b0b, 20, A_NOT,   P_8abcdSI }
,	{ 0x0b0b0b0b, 11, M_V8MIN, P_8abcdSI }
,	{ 0x0b0b0b0b, 52, A_NOT,   P_8abcdI }
,	{ 0x0b0b0b0b, 52, A_NOT,   P_8abcdSI }
,	{ 0x0c0c0c0c, 12, A_OR,    P_8abcdI }
,	{ 0x0c0c0c0c,  6, A_ADD,   P_8abcdI }
,	{ 0x0c0c0c0c, 19, A_NOT,   P_8abcdI }
,	{ 0x0c0c0c0c, 12, M_V8MIN, P_8abcdI }
,	{ 0x0c0c0c0c,  6, M_V8ADDS,P_8abcdI }
,	{ 0x0c0c0c0c, 12, A_OR,    P_8abcdSI }
,	{ 0x0c0c0c0c,  6, A_ADD,   P_8abcdSI }
,	{ 0x0c0c0c0c, 19, A_NOT,   P_8abcdSI }
,	{ 0x0c0c0c0c, 12, M_V8MIN, P_8abcdSI }
,	{ 0x0c0c0c0c,  6, M_V8ADDS,P_8abcdSI }
,	{ 0x0c0c0c0c, 51, A_NOT,   P_8abcdI }
,	{ 0x0c0c0c0c, 51, A_NOT,   P_8abcdSI }
,	{ 0x0d0d0d0d, 13, A_OR,    P_8abcdI }
,	{ 0x0d0d0d0d, 18, A_NOT,   P_8abcdI }
,	{ 0x0d0d0d0d, 13, M_V8MIN, P_8abcdI }
,	{ 0x0d0d0d0d, 13, A_OR,    P_8abcdSI }
,	{ 0x0d0d0d0d, 18, A_NOT,   P_8abcdSI }
,	{ 0x0d0d0d0d, 13, M_V8MIN, P_8abcdSI }
,	{ 0x0d0d0d0d, 50, A_NOT,   P_8abcdI }
,	{ 0x0d0d0d0d, 50, A_NOT,   P_8abcdSI }
,	{ 0x0e000000,  7, A_ROR }
,	{ 0x0e000000, 41, M_V8MULD }
,	{ 0x0e0e0e0e, 14, A_OR,    P_8abcdI }
,	{ 0x0e0e0e0e,  7, A_ADD,   P_8abcdI }
,	{ 0x0e0e0e0e, 17, A_NOT,   P_8abcdI }
,	{ 0x0e0e0e0e, 14, M_V8MIN, P_8abcdI }
,	{ 0x0e0e0e0e,  7, M_V8ADDS,P_8abcdI }
,	{ 0x0e0e0e0e, 14, A_OR,    P_8abcdI }
,	{ 0x0e0e0e0e,  7, A_ADD,   P_8abcdSI }
,	{ 0x0e0e0e0e, 17, A_NOT,   P_8abcdSI }
,	{ 0x0e0e0e0e, 14, M_V8MIN, P_8abcdSI }
,	{ 0x0e0e0e0e,  7, M_V8ADDS,P_8abcdSI }
,	{ 0x0e0e0e0e, 49, A_NOT,   P_8abcdI }
,	{ 0x0e0e0e0e, 49, A_NOT,   P_8abcdSI }
,	{ 0x0e400000, 40, M_V8MULD }
,	{ 0x0e400000, 42, M_V8MULD }
,	{ 0x0f000000, 43, M_V8MULD }
,	{ 0x0f000000, 45, M_V8MULD }
,	{ 0x0f0f0f0f, 15, A_OR,    P_8abcdI }
,	{ 0x0f0f0f0f, 16, A_NOT,   P_8abcdI }
,	{ 0x0f0f0f0f, 28, A_SHR,   P_8abcdI }
,	{ 0x0f0f0f0f, 15, M_V8MIN, P_8abcdI }
,	{ 0x0f0f0f0f, 15, A_OR,    P_8abcdSI }
,	{ 0x0f0f0f0f, 16, A_NOT,   P_8abcdSI }
,	{ 0x0f0f0f0f, 28, A_SHR,   P_8abcdSI }
,	{ 0x0f0f0f0f, 15, M_V8MIN, P_8abcdSI }
,	{ 0x0f0f0f0f, 48, A_NOT,   P_8abcdI }
,	{ 0x0f0f0f0f, 48, A_NOT,   P_8abcdSI }
,	{ 0x0f400000, 44, M_V8MULD }
,	{ 0x0f400000, 46, M_V8MULD }
,	{ 0x10000000, 47, M_V8MULD }
,	{ 0x10000000, 33, M_V8MULD }
,	{ 0x10101010,  8, A_ADD,   P_8abcdI }
,	{ 0x10101010, 36, A_FTOI,  P_8abcdI }
,	{ 0x10101010,  8, M_V8ADDS,P_8abcdI }
,	{ 0x10101010,  4, M_MUL24, P_8abcdI }
,	{ 0x10101010, 28, M_MUL24, P_8abcdI }
,	{ 0x10101010, 44, M_V8MIN, P_8abcdSF }
,	{ 0x10101010, 46, M_FMUL,  P_8abcdSF }
,	{ 0x10101010,  4, M_MUL24, P_8abcdSI }
,	{ 0x10101010, 36, A_FTOI,  P_8abcdSI }
,	{ 0x10101010,  8, M_V8ADDS,P_8abcdSI }
,	{ 0x10400000, 32, M_V8MULD }
,	{ 0x10400000, 34, M_V8MULD }
,	{ 0x11000000, 35, M_V8MULD }
,	{ 0x11000000, 37, M_V8MULD }
,	{ 0x11400000, 36, M_V8MULD }
,	{ 0x11400000, 38, M_V8MULD }
,	{ 0x12000000, 39, M_V8MULD }
,	{ 0x12121212,  9, A_ADD,   P_8abcdI }
,	{ 0x12121212,  9, M_V8ADDS,P_8abcdI }
,	{ 0x12121212,  9, A_ADD,   P_8abcdSI }
,	{ 0x12121212,  9, M_V8ADDS,P_8abcdSI }
,	{ 0x14141414, 10, A_ADD,   P_8abcdI }
,	{ 0x14141414, 10, M_V8ADDS,P_8abcdI }
,	{ 0x14141414, 10, A_ADD,   P_8abcdSI }
,	{ 0x14141414, 10, M_V8ADDS,P_8abcdSI }
,	{ 0x16161616, 11, A_ADD,   P_8abcdI }
,	{ 0x16161616, 11, M_V8ADDS,P_8abcdI }
,	{ 0x16161616, 11, A_ADD,   P_8abcdSI }
,	{ 0x16161616, 11, M_V8ADDS,P_8abcdSI }
,	{ 0x18000000,  6, A_ROR }
,	{ 0x18181818, 12, A_ADD,   P_8abcdI }
,	{ 0x18181818,  3, A_SHL,   P_8abcdI }
,	{ 0x18181818, 12, M_V8ADDS,P_8abcdI }
,	{ 0x18181818, 12, A_ADD,   P_8abcdSI }
,	{ 0x18181818,  3, A_SHL,   P_8abcdSI }
,	{ 0x18181818, 12, M_V8ADDS,P_8abcdSI }
,	{ 0x19191919,  5, M_MUL24, P_8abcdI }
,	{ 0x19191919, 27, M_MUL24, P_8abcdI }
,	{ 0x19191919,  5, M_MUL24, P_8abcdSI }
,	{ 0x1a1a1a1a, 13, A_ADD,   P_8abcdI }
,	{ 0x1a1a1a1a, 13, M_V8ADDS,P_8abcdI }
,	{ 0x1a1a1a1a, 13, A_ADD,   P_8abcdSI }
,	{ 0x1a1a1a1a, 13, M_V8ADDS,P_8abcdSI }
,	{ 0x1c1c1c1c, 14, A_ADD,   P_8abcdI }
,	{ 0x1c1c1c1c, 14, M_V8ADDS,P_8abcdI }
,	{ 0x1c1c1c1c,  8, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c,  9, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 10, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 11, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 12, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 13, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 15, A_CLZ,   P_8abcdI }
,	{ 0x1c1c1c1c, 14, A_ADD,   P_8abcdSI }
,	{ 0x1c1c1c1c, 14, M_V8ADDS,P_8abcdSI }
,	{ 0x1c1c1c1c,  8, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c,  9, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c, 10, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c, 11, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c, 12, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c, 13, A_CLZ,   P_8abcdSI }
,	{ 0x1c1c1c1c, 15, A_CLZ,   P_8abcdSI }
,	{ 0x1d1d1d1d,  4, A_CLZ,   P_8abcdI }
,	{ 0x1d1d1d1d,  5, A_CLZ,   P_8abcdI }
,	{ 0x1d1d1d1d,  6, A_CLZ,   P_8abcdI }
,	{ 0x1d1d1d1d,  7, A_CLZ,   P_8abcdI }
,	{ 0x1d1d1d1d,  4, A_CLZ,   P_8abcdSI }
,	{ 0x1d1d1d1d,  5, A_CLZ,   P_8abcdSI }
,	{ 0x1d1d1d1d,  6, A_CLZ,   P_8abcdSI }
,	{ 0x1d1d1d1d,  7, A_CLZ,   P_8abcdSI }
,	{ 0x1e1e1e1e, 15, A_ADD,   P_8abcdI }
,	{ 0x1e1e1e1e,  2, A_CLZ,   P_8abcdI }
,	{ 0x1e1e1e1e,  3, A_CLZ,   P_8abcdI }
,	{ 0x1e1e1e1e, 15, M_V8ADDS,P_8abcdI }
,	{ 0x1e1e1e1e, 15, A_ADD,   P_8abcdSI }
,	{ 0x1e1e1e1e,  2, A_CLZ,   P_8abcdSI }
,	{ 0x1e1e1e1e,  3, A_CLZ,   P_8abcdSI }
,	{ 0x1e1e1e1e, 15, M_V8ADDS,P_8abcdSI }
,	{ 0x1f1f1f1f,  1, A_CLZ,   P_8abcdI }
,	{ 0x1f1f1f1f, 27, A_SHR,   P_8abcdI }
,	{ 0x1f1f1f1f,  1, A_CLZ,   P_8abcdSI }
,	{ 0x1f1f1f1f, 27, A_SHR,   P_8abcdSI }
,	{ 0x1f1f1f1f, 59, A_SHR,   P_8abcdI }
,	{ 0x1f1f1f1f, 59, A_SHR,   P_8abcdSI }
,	{ 0x20202020, 37, A_FTOI,  P_8abcdI }
,	{ 0x20202020,  0, A_CLZ,   P_8abcdI }
,	{ 0x20202020, 45, M_V8MIN, P_8abcdSF }
,	{ 0x20202020, 37, A_FTOI,  P_8abcdSI }
,	{ 0x20202020,  0, A_CLZ,   P_8abcdSI }
,	{ 0x24242424,  6, M_MUL24, P_8abcdI }
,	{ 0x24242424, 26, M_MUL24, P_8abcdI }
,	{ 0x24242424,  6, M_MUL24, P_8abcdSI }
,	{ 0x28000000,  5, A_ROR }
,	{ 0x31313131,  7, M_MUL24, P_8abcdI }
,	{ 0x31313131, 25, M_MUL24, P_8abcdI }
,	{ 0x31313131,  7, M_MUL24, P_8abcdSI }
,	{ 0x37800000, 40, M_FMUL } // 1/65536.
,	{ 0x38800000, 41, M_FMUL } // 1/16384.
,	{ 0x39800000, 42, M_FMUL } // 1/4096.
,	{ 0x3a800000, 43, M_FMUL } // 1/1024.
,	{ 0x3b800000, 40, A_OR }   // 1/256.
,	{ 0x3b800000, 40, M_V8MIN }
,	{ 0x3b800000, 44, M_FMUL }
,	{ 0x3c000000, 41, A_OR }   // 1/128.
,	{ 0x3c000000, 40, A_FADD }
,	{ 0x3c000000, 41, M_V8MIN }
,	{ 0x3c800000, 42, A_OR }   // 1/64.
,	{ 0x3c800000, 41, A_FADD }
,	{ 0x3c800000, 42, M_V8MIN }
,	{ 0x3c800000, 45, M_FMUL }
,	{ 0x3d000000, 43, A_OR }   // 1/32.
,	{ 0x3d000000, 42, A_FADD }
,	{ 0x3d000000, 43, M_V8MIN }
,	{ 0x3d800000, 44, A_OR }   // 1/16.
,	{ 0x3d800000, 43, A_FADD }
,	{ 0x3d800000, 44, M_V8MIN }
,	{ 0x3d800000, 46, M_FMUL }
,	{ 0x3e000000, 45, A_OR }   // 1/8.
,	{ 0x3e000000, 44, A_FADD }
,	{ 0x3e000000, 45, M_V8MIN }
,	{ 0x3e800000, 46, A_OR }   // 1/4.
,	{ 0x3e800000, 45, A_FADD }
,	{ 0x3e800000, 46, M_V8MIN }
,	{ 0x3e800000, 47, M_FMUL }
,	{ 0x3f000000, 47, A_OR }   // 1/2.
,	{ 0x3f000000, 46, A_FADD }
,	{ 0x3f000000, 47, M_V8MIN }
,	{ 0x3f3f3f3f, 26, A_SHR,   P_8abcdI }
,	{ 0x3f3f3f3f, 26, A_SHR,   P_8abcdSI }
,	{ 0x3f3f3f3f, 58, A_SHR,   P_8abcdI }
,	{ 0x3f3f3f3f, 58, A_SHR,   P_8abcdSI }
,	{ 0x3f800000, 32, A_OR }   // 1.0
,	{ 0x3f800000,  1, A_ITOF }
,	{ 0x3f800000, 47, A_FADD }
,	{ 0x3f800000, 32, M_V8MIN }
,	{ 0x40000000, 33, A_OR }   // 2.0
,	{ 0x40000000,  2, A_ITOF }
,	{ 0x40000000, 32, A_FADD }
,	{ 0x40000000,  4, A_ROR }
,	{ 0x40000000, 33, M_V8MIN }
,	{ 0x40400000,  3, A_ITOF } // 3.0
,	{ 0x40404040, 38, A_FTOI,  P_8abcdI }
,	{ 0x40404040,  4, A_SHL,   P_8abcdI }
,	{ 0x40404040,  8, M_MUL24, P_8abcdI }
,	{ 0x40404040, 24, M_MUL24, P_8abcdI }
,	{ 0x40404040, 46, M_V8MIN, P_8abcdSF }
,	{ 0x40404040, 47, M_FMUL,  P_8abcdSF }
,	{ 0x40404040, 38, A_FTOI,  P_8abcdSI }
,	{ 0x40404040,  4, A_SHL,   P_8abcdSI }
,	{ 0x40404040,  8, M_MUL24, P_8abcdSI }
,	{ 0x40800000, 34, A_OR }   // 4.0
,	{ 0x40800000,  4, A_ITOF }
,	{ 0x40800000, 33, A_FADD }
,	{ 0x40800000, 34, M_V8MIN }
,	{ 0x40800000, 33, M_FMUL }
,	{ 0x40a00000,  5, A_ITOF } // 5.0
,	{ 0x40c00000,  6, A_ITOF } // 6.0
,	{ 0x40e00000,  7, A_ITOF } // 7.0
,	{ 0x41000000, 35, A_OR }   // 8.0
,	{ 0x41000000,  8, A_ITOF }
,	{ 0x41000000, 34, A_FADD }
,	{ 0x41000000, 35, M_V8MIN }
,	{ 0x41100000,  9, A_ITOF } // 9.0
,	{ 0x41200000, 10, A_ITOF } // 10.0
,	{ 0x41300000, 11, A_ITOF } // 11.0
,	{ 0x41400000, 12, A_ITOF } // 12.0
,	{ 0x41500000, 13, A_ITOF } // 13.0
,	{ 0x41600000, 14, A_ITOF } // 14.0
,	{ 0x41700000, 15, A_ITOF } // 15.0
,	{ 0x41800000, 36, A_OR }   // 16.0
,	{ 0x41800000, 35, A_FADD }
,	{ 0x41800000, 36, M_V8MIN }
,	{ 0x41800000, 34, M_FMUL }
,	{ 0x42000000, 37, A_OR }   // 32.0
,	{ 0x42000000, 36, A_FADD }
,	{ 0x42000000, 37, M_V8MIN }
,	{ 0x42800000, 38, A_OR }   // 64.0
,	{ 0x42800000, 37, A_FADD }
,	{ 0x42800000, 38, M_V8MIN }
,	{ 0x42800000, 35, M_FMUL }
,	{ 0x43000000, 39, A_OR }   // 128.0
,	{ 0x43000000, 38, A_FADD }
,	{ 0x43000000, 39, M_V8MIN }
,	{ 0x43800000, 39, A_FADD } // 256.0
,	{ 0x43800000, 36, M_FMUL }
,	{ 0x44800000, 37, M_FMUL } // 1024.0
,	{ 0x45800000, 38, M_FMUL } // 4096.0
,	{ 0x46800000, 39, M_FMUL } // 16384.0
,	{ 0x4e6e0000, 40, A_ITOF }
,	{ 0x4e700000, 41, A_ITOF }
,	{ 0x4e720000, 42, A_ITOF }
,	{ 0x4e740000, 43, A_ITOF }
,	{ 0x4e760000, 44, A_ITOF }
,	{ 0x4e780000, 45, A_ITOF }
,	{ 0x4e7a0000, 46, A_ITOF }
,	{ 0x4e7c0000, 47, A_ITOF }
,	{ 0x4e7e0000, 32, A_ITOF }
,	{ 0x4e800000, 33, A_ITOF }
,	{ 0x4e810000, 34, A_ITOF }
,	{ 0x4e820000, 35, A_ITOF }
,	{ 0x4e830000, 36, A_ITOF }
,	{ 0x4e840000, 37, A_ITOF }
,	{ 0x4e850000, 38, A_ITOF }
,	{ 0x4e860000, 39, A_ITOF }
,	{ 0x51515151,  9, M_MUL24, P_8abcdI }
,	{ 0x51515151, 23, M_MUL24, P_8abcdI }
,	{ 0x51515151,  9, M_MUL24, P_8abcdSI }
,	{ 0x60000000,  3, A_ROR }
,	{ 0x64646464, 10, M_MUL24, P_8abcdI }
,	{ 0x64646464, 22, M_MUL24, P_8abcdI }
,	{ 0x64646464, 10, M_MUL24, P_8abcdSI }
,	{ 0x76ff0000, 40, M_V8ADDS }
,	{ 0x76ff0000, 40, A_V8ADDS }
,	{ 0x77000000, 40, A_ADD }
,	{ 0x78000000, 41, M_V8ADDS }
,	{ 0x78000000, 41, A_ADD }
,	{ 0x78ff0000, 42, M_V8ADDS }
,	{ 0x78ff0000, 42, A_V8ADDS }
,	{ 0x79000000, 42, A_ADD }
,	{ 0x79797979, 11, M_MUL24, P_8abcdI }
,	{ 0x79797979, 21, M_MUL24, P_8abcdI }
,	{ 0x79797979, 11, M_MUL24, P_8abcdSI }
,	{ 0x7a000000, 43, M_V8ADDS }
,	{ 0x7a000000, 43, A_ADD }
,	{ 0x7aff0000, 44, M_V8ADDS }
,	{ 0x7aff0000, 44, A_V8ADDS }
,	{ 0x7b000000, 44, A_ADD }
,	{ 0x7c000000, 45, M_V8ADDS }
,	{ 0x7c000000, 45, A_ADD }
,	{ 0x7cff0000, 46, M_V8ADDS }
,	{ 0x7cff0000, 46, A_V8ADDS }
,	{ 0x7d000000, 46, A_ADD }
,	{ 0x7e000000, 47, M_V8ADDS }
,	{ 0x7e000000, 47, A_ADD }
,	{ 0x7eff0000, 32, M_V8ADDS }
,	{ 0x7eff0000, 32, A_V8ADDS }
,	{ 0x7f000000, 32, A_ADD }
,	{ 0x7f7f7f7f, 25, A_SHR,   P_8abcdI }
,	{ 0x7f7f7f7f, 27, A_ROR,   P_8abcdI }
,	{ 0x7f7f7f7f, 25, A_SHR,   P_8abcdSI }
,	{ 0x7f7f7f7f, 57, A_SHR,   P_8abcdI }
,	{ 0x7f7f7f7f, 59, A_ROR,   P_8abcdI }
,	{ 0x7f7f7f7f, 57, A_SHR,   P_8abcdSI }
,	{ 0x7f800000, 16, A_FSUB } // +Inf
,	{ 0x7f800000, 16, M_FMUL }
,	{ 0x7f800000, 17, A_FSUB }
,	{ 0x7f800000, 17, M_FMUL }
,	{ 0x7f800000, 18, A_FSUB }
,	{ 0x7f800000, 18, M_FMUL }
,	{ 0x7f800000, 19, A_FSUB }
,	{ 0x7f800000, 19, M_FMUL }
,	{ 0x7f800000, 20, A_FSUB }
,	{ 0x7f800000, 20, M_FMUL }
,	{ 0x7f800000, 21, A_FSUB }
,	{ 0x7f800000, 21, M_FMUL }
,	{ 0x7f800000, 22, A_FSUB }
,	{ 0x7f800000, 22, M_FMUL }
,	{ 0x7f800000, 23, A_FSUB }
,	{ 0x7f800000, 23, M_FMUL }
,	{ 0x7f800000, 24, A_FSUB }
,	{ 0x7f800000, 24, M_FMUL }
,	{ 0x7f800000, 25, A_FSUB }
,	{ 0x7f800000, 25, M_FMUL }
,	{ 0x7f800000, 26, A_FSUB }
,	{ 0x7f800000, 26, M_FMUL }
,	{ 0x7f800000, 27, A_FSUB }
,	{ 0x7f800000, 27, M_FMUL }
,	{ 0x7f800000, 28, A_FSUB }
,	{ 0x7f800000, 28, M_FMUL }
,	{ 0x7f800000, 29, A_FSUB }
,	{ 0x7f800000, 29, M_FMUL }
,	{ 0x7f800000, 30, A_FSUB }
,	{ 0x7f800000, 30, M_FMUL }
,	{ 0x7f800000, 31, A_FSUB }
,	{ 0x7f800000, 31, M_FMUL }
,	{ 0x7f800000, 48, A_FSUB }
,	{ 0x7f800000, 49, A_FSUB }
,	{ 0x7f800000, 50, A_FSUB }
,	{ 0x7f800000, 51, A_FSUB }
,	{ 0x7f800000, 52, A_FSUB }
,	{ 0x7f800000, 53, A_FSUB }
,	{ 0x7f800000, 54, A_FSUB }
,	{ 0x7f800000, 55, A_FSUB }
,	{ 0x7f800000, 56, A_FSUB }
,	{ 0x7f800000, 57, A_FSUB }
,	{ 0x7f800000, 58, A_FSUB }
,	{ 0x7f800000, 59, A_FSUB }
,	{ 0x7f800000, 60, A_FSUB }
,	{ 0x7f800000, 61, A_FSUB }
,	{ 0x7f800000, 62, A_FSUB }
,	{ 0x7f800000, 63, A_FSUB }
,	{ 0x80000000,  1, A_ROR } // INT_MIN
,	{ 0x80000000, 33, M_V8ADDS }
,	{ 0x80000000, 33, A_ADD }
,	{ 0x80000000, 30, A_SHL }
,	{ 0x80000000, 62, A_SHL }
,	{ 0x80808080, 39, A_FTOI,  P_8abcdI }
,	{ 0x80808080,  6, A_SHL,   P_8abcdI }
,	{ 0x80808080,  7, A_SHL,   P_8abcdI }
,	{ 0x80808080, 47, M_V8MIN, P_8abcdSF }
,	{ 0x80808080, 39, A_FTOI,  P_8abcdSI }
,	{ 0x80ff0000, 34, M_V8ADDS }
,	{ 0x80ff0000, 34, A_V8ADDS }
,	{ 0x81000000, 34, A_ADD }
,	{ 0x82000000, 35, M_V8ADDS }
,	{ 0x82000000, 35, A_ADD }
,	{ 0x82ff0000, 36, M_V8ADDS }
,	{ 0x82ff0000, 36, A_V8ADDS }
,	{ 0x83000000, 36, A_ADD }
,	{ 0x84000000, 37, M_V8ADDS }
,	{ 0x84000000, 37, A_ADD }
,	{ 0x84ff0000, 38, M_V8ADDS }
,	{ 0x84ff0000, 38, A_V8ADDS }
,	{ 0x85000000, 38, A_ADD }
,	{ 0x86000000, 39, M_V8ADDS }
,	{ 0x86000000, 39, A_ADD }
,	{ 0x90909090, 12, M_MUL24, P_8abcdI }
,	{ 0x90909090, 20, M_MUL24, P_8abcdI }
,	{ 0x90909090, 12, M_MUL24, P_8abcdSI }
,	{ 0xa0000000, 29, A_SHL }
,	{ 0xa0000000, 61, A_SHL }
,	{ 0xa0a0a0a0,  5, A_SHL,   P_8abcdI }
,	{ 0xa0a0a0a0,  5, A_SHL,   P_8abcdSI }
,	{ 0xa9a9a9a9, 13, M_MUL24, P_8abcdI }
,	{ 0xa9a9a9a9, 19, M_MUL24, P_8abcdI }
,	{ 0xa9a9a9a9, 13, M_MUL24, P_8abcdSI }
,	{ 0xbcffffff, 39, A_NOT }
,	{ 0xbd7fffff, 38, A_NOT }
,	{ 0xbdffffff, 37, A_NOT }
,	{ 0xbe7fffff, 36, A_NOT }
,	{ 0xbeffffff, 35, A_NOT }
,	{ 0xbf7fffff, 34, A_NOT }
,	{ 0xbf800000, 31, A_ITOF } // -1.0
,	{ 0xbf800000, 63, A_ITOF }
,	{ 0xbfbfbfbf, 26, A_ROR,   P_8abcdI }
,	{ 0xbfbfbfbf, 58, A_ROR,   P_8abcdI }
,	{ 0xbfffffff, 33, A_NOT }
,	{ 0xc0000000, 30, A_ITOF } // -2.0
,	{ 0xc0000000, 28, A_SHL }
,	{ 0xc0000000, 62, A_ITOF }
,	{ 0xc0000000, 60, A_SHL }
,	{ 0xc0400000, 29, A_ITOF } // -3.0
,	{ 0xc0400000, 61, A_ITOF }
,	{ 0xc07fffff, 32, A_NOT }
,	{ 0xc0800000, 28, A_ITOF } // -4.0
,	{ 0xc0800000, 60, A_ITOF }
,	{ 0xc0a00000, 27, A_ITOF } // -5.0
,	{ 0xc0a00000, 59, A_ITOF }
,	{ 0xc0c00000, 26, A_ITOF } // -6.0
,	{ 0xc0c00000, 58, A_ITOF }
,	{ 0xc0e00000, 25, A_ITOF } // -7.0
,	{ 0xc0e00000, 57, A_ITOF }
,	{ 0xc0ffffff, 47, A_NOT }
,	{ 0xc1000000, 24, A_ITOF } // -8.0
,	{ 0xc1000000, 56, A_ITOF }
,	{ 0xc1100000, 23, A_ITOF } // -9.0
,	{ 0xc1100000, 55, A_ITOF }
,	{ 0xc1200000, 22, A_ITOF } // -10.0
,	{ 0xc1200000, 54, A_ITOF }
,	{ 0xc1300000, 21, A_ITOF } // -11.0
,	{ 0xc1300000, 53, A_ITOF }
,	{ 0xc1400000, 20, A_ITOF } // -12.0
,	{ 0xc1400000, 52, A_ITOF }
,	{ 0xc1500000, 19, A_ITOF } // -13.0
,	{ 0xc1500000, 51, A_ITOF }
,	{ 0xc1600000, 18, A_ITOF } // -14.0
,	{ 0xc1600000, 50, A_ITOF }
,	{ 0xc1700000, 17, A_ITOF } // -15.0
,	{ 0xc1700000, 49, A_ITOF }
,	{ 0xc17fffff, 46, A_NOT }
,	{ 0xc1800000, 16, A_ITOF } // -16.0
,	{ 0xc1800000, 48, A_ITOF }
,	{ 0xc1ffffff, 45, A_NOT }
,	{ 0xc27fffff, 44, A_NOT }
,	{ 0xc2ffffff, 43, A_NOT }
,	{ 0xc37fffff, 42, A_NOT }
,	{ 0xc3ffffff, 41, A_NOT }
,	{ 0xc4c4c4c4, 14, M_MUL24, P_8abcdI }
,	{ 0xc4c4c4c4, 18, M_MUL24, P_8abcdI }
,	{ 0xc4c4c4c4, 14, M_MUL24, P_8abcdSI }
,	{ 0xc47fffff, 40, A_NOT }
,	{ 0xcfcfcfcf, 28, A_ROR,   P_8abcdI }
,	{ 0xcfcfcfcf, 60, A_ROR,   P_8abcdI }
,	{ 0xd8000000, 27, A_SHL }
,	{ 0xd8000000, 59, A_SHL }
,	{ 0xe0000100, 16, M_MUL24 }
,	{ 0xe0e0e0e0, 16, A_ADD,   P_8abcdI }
,	{ 0xe0e0e0e0, 48, A_ADD,   P_8abcdI }
,	{ 0xe1e1e1e1, 15, M_MUL24, P_8abcdI }
,	{ 0xe1e1e1e1, 17, M_MUL24, P_8abcdI }
,	{ 0xe1e1e1e1, 15, M_MUL24, P_8abcdSI }
,	{ 0xe20000e1, 17, M_MUL24 }
,	{ 0xe2e2e2e2, 17, A_ADD,   P_8abcdI }
,	{ 0xe2e2e2e2, 16, M_V8MULD,P_8abcdI }
,	{ 0xe2e2e2e2, 49, A_ADD,   P_8abcdI }
,	{ 0xe40000c4, 18, M_MUL24 }
,	{ 0xe4e4e4e4, 18, A_ADD,   P_8abcdI }
,	{ 0xe4e4e4e4, 17, M_V8MULD,P_8abcdI }
,	{ 0xe4e4e4e4, 50, A_ADD,   P_8abcdI }
,	{ 0xe60000a9, 19, M_MUL24 }
,	{ 0xe6e6e6e6, 19, A_ADD,   P_8abcdI }
,	{ 0xe6e6e6e6, 18, M_V8MULD,P_8abcdI }
,	{ 0xe6e6e6e6, 51, A_ADD,   P_8abcdI }
,	{ 0xe8000000, 26, A_SHL }
,	{ 0xe8000000, 58, A_SHL }
,	{ 0xe8000090, 20, M_MUL24 }
,	{ 0xe8e8e8e8, 20, A_ADD,   P_8abcdI }
,	{ 0xe8e8e8e8, 19, M_V8MULD,P_8abcdI }
,	{ 0xe8e8e8e8, 52, A_ADD,   P_8abcdI }
,	{ 0xe9e9e9e9, 20, M_V8MULD,P_8abcdI }
,	{ 0xea000079, 21, M_MUL24 }
,	{ 0xeaeaeaea, 21, A_ADD,   P_8abcdI }
,	{ 0xeaeaeaea, 53, A_ADD,   P_8abcdI }
,	{ 0xebebebeb, 21, M_V8MULD,P_8abcdI }
,	{ 0xec000064, 22, M_MUL24 }
,	{ 0xecececec, 22, A_ADD,   P_8abcdI }
,	{ 0xecececec, 54, A_ADD,   P_8abcdI }
,	{ 0xedededed, 22, M_V8MULD,P_8abcdI }
,	{ 0xee000051, 23, M_MUL24 }
,	{ 0xeeeeeeee, 23, A_ADD,   P_8abcdI }
,	{ 0xeeeeeeee, 55, A_ADD,   P_8abcdI }
,	{ 0xefefefef, 29, A_ROR,   P_8abcdI }
,	{ 0xefefefef, 23, M_V8MULD,P_8abcdI }
,	{ 0xefefefef, 61, A_ROR,   P_8abcdI }
,	{ 0xf0000040, 24, M_MUL24 }
,	{ 0xf0f0f0f0, 16, A_OR,    P_8abcdI }
,	{ 0xf0f0f0f0, 24, A_ADD,   P_8abcdI }
,	{ 0xf0f0f0f0, 15, A_NOT,   P_8abcdI }
,	{ 0xf0f0f0f0, 16, M_V8MIN, P_8abcdI }
,	{ 0xf0f0f0f0, 48, A_OR,    P_8abcdI }
,	{ 0xf0f0f0f0, 56, A_ADD,   P_8abcdI }
,	{ 0xf1f1f1f1, 17, A_OR,    P_8abcdI }
,	{ 0xf1f1f1f1, 14, A_NOT,   P_8abcdI }
,	{ 0xf1f1f1f1, 17, M_V8MIN, P_8abcdI }
,	{ 0xf1f1f1f1, 24, M_V8MULD,P_8abcdI }
,	{ 0xf1f1f1f1, 49, A_OR,    P_8abcdI }
,	{ 0xf2000000, 25, A_SHL }
,	{ 0xf2000000, 57, A_SHL }
,	{ 0xf2000031, 25, M_MUL24 }
,	{ 0xf2f2f2f2, 18, A_OR,    P_8abcdI }
,	{ 0xf2f2f2f2, 25, A_ADD,   P_8abcdI }
,	{ 0xf2f2f2f2, 13, A_NOT,   P_8abcdI }
,	{ 0xf2f2f2f2, 18, M_V8MIN, P_8abcdI }
,	{ 0xf2f2f2f2, 50, A_OR,    P_8abcdI }
,	{ 0xf2f2f2f2, 57, A_ADD,   P_8abcdI }
,	{ 0xf3f3f3f3, 19, A_OR,    P_8abcdI }
,	{ 0xf3f3f3f3, 12, A_NOT,   P_8abcdI }
,	{ 0xf3f3f3f3, 19, M_V8MIN, P_8abcdI }
,	{ 0xf3f3f3f3, 25, M_V8MULD,P_8abcdI }
,	{ 0xf3f3f3f3, 51, A_OR,    P_8abcdI }
,	{ 0xf4000024, 26, M_MUL24 }
,	{ 0xf4f4f4f4, 20, A_OR,    P_8abcdI }
,	{ 0xf4f4f4f4, 26, A_ADD,   P_8abcdI }
,	{ 0xf4f4f4f4, 11, A_NOT,   P_8abcdI }
,	{ 0xf4f4f4f4, 20, M_V8MIN, P_8abcdI }
,	{ 0xf4f4f4f4, 52, A_OR,    P_8abcdI }
,	{ 0xf4f4f4f4, 58, A_ADD,   P_8abcdI }
,	{ 0xf5f5f5f5, 21, A_OR,    P_8abcdI }
,	{ 0xf5f5f5f5, 10, A_NOT,   P_8abcdI }
,	{ 0xf5f5f5f5, 21, M_V8MIN, P_8abcdI }
,	{ 0xf5f5f5f5, 26, M_V8MULD,P_8abcdI }
,	{ 0xf5f5f5f5, 53, A_OR,    P_8abcdI }
,	{ 0xf6000019, 27, M_MUL24 }
,	{ 0xf6f6f6f6, 22, A_OR,    P_8abcdI }
,	{ 0xf6f6f6f6, 27, A_ADD,   P_8abcdI }
,	{ 0xf6f6f6f6,  9, A_NOT,   P_8abcdI }
,	{ 0xf6f6f6f6, 22, M_V8MIN, P_8abcdI }
,	{ 0xf6f6f6f6, 54, A_OR,    P_8abcdI }
,	{ 0xf6f6f6f6, 59, A_ADD,   P_8abcdI }
,	{ 0xf7f7f7f7, 23, A_OR,    P_8abcdI }
,	{ 0xf7f7f7f7,  8, A_NOT,   P_8abcdI }
,	{ 0xf7f7f7f7, 23, M_V8MIN, P_8abcdI }
,	{ 0xf7f7f7f7, 27, M_V8MULD,P_8abcdI }
,	{ 0xf7f7f7f7, 55, A_OR,    P_8abcdI }
,	{ 0xf8000000, 24, A_SHL }
,	{ 0xf8000000, 56, A_SHL }
,	{ 0xf8000010, 28, M_MUL24 }
,	{ 0xf8f8f8f8, 24, A_OR,    P_8abcdI }
,	{ 0xf8f8f8f8, 28, A_ADD,   P_8abcdI }
,	{ 0xf8f8f8f8,  7, A_NOT,   P_8abcdI }
,	{ 0xf8f8f8f8, 24, M_V8MIN, P_8abcdI }
,	{ 0xf8f8f8f8, 56, A_OR,    P_8abcdI }
,	{ 0xf8f8f8f8, 60, A_ADD,   P_8abcdI }
,	{ 0xf9f9f9f9, 25, A_OR,    P_8abcdI }
,	{ 0xf9f9f9f9,  6, A_NOT,   P_8abcdI }
,	{ 0xf9f9f9f9, 25, M_V8MIN, P_8abcdI }
,	{ 0xf9f9f9f9, 28, M_V8MULD,P_8abcdI }
,	{ 0xf9f9f9f9, 57, A_OR,    P_8abcdI }
,	{ 0xfa000009, 29, M_MUL24 }
,	{ 0xfafafafa, 26, A_OR,    P_8abcdI }
,	{ 0xfafafafa, 29, A_ADD,   P_8abcdI }
,	{ 0xfafafafa,  5, A_NOT,   P_8abcdI }
,	{ 0xfafafafa, 26, M_V8MIN, P_8abcdI }
,	{ 0xfafafafa, 58, A_OR,    P_8abcdI }
,	{ 0xfafafafa, 61, A_ADD,   P_8abcdI }
,	{ 0xfb800000, 23, A_SHL }
,	{ 0xfbfbfbfb, 27, A_OR,    P_8abcdI }
,	{ 0xfbfbfbfb,  4, A_NOT,   P_8abcdI }
,	{ 0xfbfbfbfb, 30, A_ROR,   P_8abcdI }
,	{ 0xfbfbfbfb, 27, M_V8MIN, P_8abcdI }
,	{ 0xfbfbfbfb, 29, M_V8MULD,P_8abcdI }
,	{ 0xfbfbfbfb, 59, A_OR,    P_8abcdI }
,	{ 0xfc000004, 30, M_MUL24 }
,	{ 0xfcfcfcfc, 28, A_OR,    P_8abcdI }
,	{ 0xfcfcfcfc, 30, A_ADD,   P_8abcdI }
,	{ 0xfcfcfcfc,  3, A_NOT,   P_8abcdI }
,	{ 0xfcfcfcfc, 28, M_V8MIN, P_8abcdI }
,	{ 0xfcfcfcfc, 60, A_OR,    P_8abcdI }
,	{ 0xfcfcfcfc, 62, A_ADD,   P_8abcdI }
,	{ 0xfd800000, 22, A_SHL }
,	{ 0xfd800000, 54, A_SHL }
,	{ 0xfdfdfdfd, 29, A_OR,    P_8abcdI }
,	{ 0xfdfdfdfd,  2, A_NOT,   P_8abcdI }
,	{ 0xfdfdfdfd, 29, M_V8MIN, P_8abcdI }
,	{ 0xfdfdfdfd, 30, M_V8MULD,P_8abcdI }
,	{ 0xfdfdfdfd, 61, A_OR,    P_8abcdI }
,	{ 0xfe000001, 31, M_MUL24 }
,	{ 0xfea00000, 21, A_SHL }
,	{ 0xfea00000, 53, A_SHL }
,	{ 0xfefefefe, 30, A_OR,    P_8abcdI }
,	{ 0xfefefefe, 30, M_V8MIN, P_8abcdI }
,	{ 0xfefefefe, 31, A_ADD,   P_8abcdI }
,	{ 0xfefefefe,  1, A_NOT,   P_8abcdI }
,	{ 0xfefefefe, 62, A_OR,    P_8abcdI }
,	{ 0xfefefefe, 63, A_ADD,   P_8abcdI }
,	{ 0xff400000, 20, A_SHL }
,	{ 0xff400000, 32, A_SHL }
,	{ 0xff800000, 16, A_FADD } // -Inf
,	{ 0xff800000, 17, A_FADD }
,	{ 0xff800000, 18, A_FADD }
,	{ 0xff800000, 19, A_FADD }
,	{ 0xff800000, 20, A_FADD }
,	{ 0xff800000, 21, A_FADD }
,	{ 0xff800000, 22, A_FADD }
,	{ 0xff800000, 23, A_FADD }
,	{ 0xff800000, 24, A_FADD }
,	{ 0xff800000, 25, A_FADD }
,	{ 0xff800000, 26, A_FADD }
,	{ 0xff800000, 27, A_FADD }
,	{ 0xff800000, 28, A_FADD }
,	{ 0xff800000, 29, A_FADD }
,	{ 0xff800000, 30, A_FADD }
,	{ 0xff800000, 31, A_FADD }
,	{ 0xff800000, 48, A_FADD }
,	{ 0xff800000, 49, A_FADD }
,	{ 0xff800000, 50, A_FADD }
,	{ 0xff800000, 51, A_FADD }
,	{ 0xff800000, 52, A_FADD }
,	{ 0xff800000, 53, A_FADD }
,	{ 0xff800000, 54, A_FADD }
,	{ 0xff800000, 55, A_FADD }
,	{ 0xff800000, 56, A_FADD }
,	{ 0xff800000, 57, A_FADD }
,	{ 0xff800000, 58, A_FADD }
,	{ 0xff800000, 59, A_FADD }
,	{ 0xff800000, 60, A_FADD }
,	{ 0xff800000, 61, A_FADD }
,	{ 0xff800000, 62, A_FADD }
,	{ 0xff800000, 63, A_FADD }
,	{ 0xff980000, 19, A_SHL }
,	{ 0xff980000, 51, A_SHL }
,	{ 0xffc80000, 18, A_SHL }
,	{ 0xffc80000, 50, A_SHL }
,	{ 0xffe20000, 17, A_SHL }
,	{ 0xffe20000, 49, A_SHL }
,	{ 0xfff00000, 16, A_SHL }
,	{ 0xfff00000, 48, A_SHL }
,	{ 0xfff0ffff, 16, A_ROR }
,	{ 0xfff0ffff, 48, A_ROR }
,	{ 0xfff8ffff, 17, A_ROR }
,	{ 0xfff8ffff, 49, A_ROR }
,	{ 0xfffcbfff, 18, A_ROR }
,	{ 0xfffcbfff, 50, A_ROR }
,	{ 0xfffe7fff, 19, A_ROR }
,	{ 0xfffe7fff, 51, A_ROR }
,	{ 0xffff4fff, 20, A_ROR }
,	{ 0xffff4fff, 52, A_ROR }
,	{ 0xffffafff, 21, A_ROR }
,	{ 0xffffafff, 53, A_ROR }
,	{ 0xffffdbff, 22, A_ROR }
,	{ 0xffffdbff, 54, A_ROR }
,	{ 0xffffefff, 23, A_ROR } // -4097
,	{ 0xffffefff, 55, A_ROR }
,	{ 0xfffff8ff, 24, A_ROR } // -1793
,	{ 0xfffff8ff, 56, A_ROR }
,	{ 0xfffffcff, 25, A_ROR } // -769
,	{ 0xfffffcff, 57, A_ROR }
,	{ 0xfffffebf, 26, A_ROR } // -321
,	{ 0xfffffebf, 58, A_ROR }
,	{ 0xffffff7f, 27, A_ROR } // -129
,	{ 0xffffff7f, 59, A_ROR }
,	{ 0xffffffcf, 28, A_ROR } // -49
,	{ 0xffffffcf, 60, A_ROR }
,	{ 0xffffffe0, 16, A_ADD } // -32
,	{ 0xffffffe0, 48, A_ADD }
,	{ 0xffffffe2, 17, A_ADD } // -30
,	{ 0xffffffe2, 16, M_V8MULD }
,	{ 0xffffffe2, 49, A_ADD }
,	{ 0xffffffe4, 18, A_ADD } // -28
,	{ 0xffffffe4, 17, M_V8MULD }
,	{ 0xffffffe4, 50, A_ADD }
,	{ 0xffffffe6, 19, A_ADD } // -26
,	{ 0xffffffe6, 18, M_V8MULD }
,	{ 0xffffffe6, 51, A_ADD }
,	{ 0xffffffe8, 20, A_ADD } // -24
,	{ 0xffffffe8, 19, M_V8MULD }
,	{ 0xffffffe8, 52, A_ADD }
,	{ 0xffffffe9, 20, M_V8MULD } // -23
,	{ 0xffffffea, 21, A_ADD }    // -22
,	{ 0xffffffea, 53, A_ADD }
,	{ 0xffffffeb, 21, M_V8MULD } // -21
,	{ 0xffffffec, 22, A_ADD }    // -20
,	{ 0xffffffec, 54, A_ADD }
,	{ 0xffffffed, 22, M_V8MULD } // -19
,	{ 0xffffffee, 23, A_ADD }    // -18
,	{ 0xffffffee, 55, A_ADD }
,	{ 0xffffffef, 29, A_ROR }    // -17
,	{ 0xffffffef, 23, M_V8MULD }
,	{ 0xffffffef, 61, A_ROR }
,	{ 0xfffffff0, 16, A_OR }     // -16
,	{ 0xfffffff0, 24, A_ADD }
,	{ 0xfffffff0, 15, A_NOT }
,	{ 0xfffffff0, 16, M_V8MIN }
,	{ 0xfffffff0, 48, A_OR }
,	{ 0xfffffff0, 56, A_ADD }
,	{ 0xfffffff1, 17, A_OR }  // -15
,	{ 0xfffffff1, 17, M_V8MIN }
,	{ 0xfffffff1, 14, A_NOT }
,	{ 0xfffffff1, 24, M_V8MULD }
,	{ 0xfffffff1, 49, A_OR }
,	{ 0xfffffff2, 18, A_OR }  // -14
,	{ 0xfffffff2, 18, M_V8MIN }
,	{ 0xfffffff2, 25, A_ADD }
,	{ 0xfffffff2, 13, A_NOT }
,	{ 0xfffffff2, 50, A_OR }
,	{ 0xfffffff2, 57, A_ADD }
,	{ 0xfffffff3, 19, A_OR }  // -13
,	{ 0xfffffff3, 19, M_V8MIN }
,	{ 0xfffffff3, 12, A_NOT }
,	{ 0xfffffff3, 25, M_V8MULD }
,	{ 0xfffffff3, 51, A_OR }
,	{ 0xfffffff4, 20, A_OR }  // -12
,	{ 0xfffffff4, 20, M_V8MIN }
,	{ 0xfffffff4, 26, A_ADD }
,	{ 0xfffffff4, 11, A_NOT }
,	{ 0xfffffff4, 52, A_OR }
,	{ 0xfffffff4, 58, A_ADD }
,	{ 0xfffffff5, 21, A_OR }  // -11
,	{ 0xfffffff5, 21, M_V8MIN }
,	{ 0xfffffff5, 10, A_NOT }
,	{ 0xfffffff5, 26, M_V8MULD }
,	{ 0xfffffff5, 53, A_OR }
,	{ 0xfffffff6, 22, A_OR }  // -10
,	{ 0xfffffff6, 22, M_V8MIN }
,	{ 0xfffffff6, 27, A_ADD }
,	{ 0xfffffff6,  9, A_NOT }
,	{ 0xfffffff6, 54, A_OR }
,	{ 0xfffffff6, 59, A_ADD }
,	{ 0xfffffff7, 23, A_OR }  // -9
,	{ 0xfffffff7, 23, M_V8MIN }
,	{ 0xfffffff7,  8, A_NOT }
,	{ 0xfffffff7, 27, M_V8MULD }
,	{ 0xfffffff7, 55, A_OR }
,	{ 0xfffffff8, 24, A_OR }  // -8
,	{ 0xfffffff8, 24, M_V8MIN }
,	{ 0xfffffff8, 28, A_ADD }
,	{ 0xfffffff8,  7, A_NOT }
,	{ 0xfffffff8, 56, A_OR }
,	{ 0xfffffff8, 60, A_ADD }
,	{ 0xfffffff9, 25, A_OR }  // -7
,	{ 0xfffffff9, 25, M_V8MIN }
,	{ 0xfffffff9,  6, A_NOT }
,	{ 0xfffffff9, 28, M_V8MULD }
,	{ 0xfffffff9, 57, A_OR }
,	{ 0xfffffffa, 26, A_OR }  // -6
,	{ 0xfffffffa, 26, M_V8MIN }
,	{ 0xfffffffa, 29, A_ADD }
,	{ 0xfffffffa,  5, A_NOT }
,	{ 0xfffffffa, 58, A_OR }
,	{ 0xfffffffa, 61, A_ADD }
,	{ 0xfffffffb, 27, A_OR }  // -5
,	{ 0xfffffffb, 27, M_V8MIN }
,	{ 0xfffffffb,  4, A_NOT }
,	{ 0xfffffffb, 30, A_ROR }
,	{ 0xfffffffb, 29, M_V8MULD }
,	{ 0xfffffffb, 59, A_OR }
,	{ 0xfffffffb, 62, A_ROR }
,	{ 0xfffffffc, 28, A_OR }  // -4
,	{ 0xfffffffc, 28, M_V8MIN }
,	{ 0xfffffffc, 30, A_ADD }
,	{ 0xfffffffc,  3, A_NOT }
,	{ 0xfffffffc, 60, A_OR }
,	{ 0xfffffffc, 62, A_ADD }
,	{ 0xfffffffd, 29, A_OR }  // -3
,	{ 0xfffffffd, 29, M_V8MIN }
,	{ 0xfffffffd,  2, A_NOT }
,	{ 0xfffffffd, 30, M_V8MULD }
,	{ 0xfffffffd, 61, A_OR }
,	{ 0xfffffffe, 30, A_OR }  // -2
,	{ 0xfffffffe, 30, M_V8MIN }
,	{ 0xfffffffe, 31, A_ADD }
,	{ 0xfffffffe,  1, A_NOT }
,	{ 0xfffffffe, 62, A_OR }  // -2
,	{ 0xfffffffe, 63, A_ADD }
,	{ 0xffffffff, 31, A_OR }  // -1
,	{ 0xffffffff, 31, M_V8MIN }
,	{ 0xffffffff,  0, A_NOT }
,	{ 0xffffffff, 16, A_V8ADDS }
,	{ 0xffffffff, 17, A_V8ADDS }
,	{ 0xffffffff, 18, A_V8ADDS }
,	{ 0xffffffff, 19, A_V8ADDS }
,	{ 0xffffffff, 20, A_V8ADDS }
,	{ 0xffffffff, 21, A_V8ADDS }
,	{ 0xffffffff, 22, A_V8ADDS }
,	{ 0xffffffff, 23, A_V8ADDS }
,	{ 0xffffffff, 24, A_V8ADDS }
,	{ 0xffffffff, 25, A_V8ADDS }
,	{ 0xffffffff, 26, A_V8ADDS }
,	{ 0xffffffff, 27, A_V8ADDS }
,	{ 0xffffffff, 28, A_V8ADDS }
,	{ 0xffffffff, 29, A_V8ADDS }
,	{ 0xffffffff, 30, A_V8ADDS }
,	{ 0xffffffff, 16, M_V8ADDS }
,	{ 0xffffffff, 17, M_V8ADDS }
,	{ 0xffffffff, 18, M_V8ADDS }
,	{ 0xffffffff, 19, M_V8ADDS }
,	{ 0xffffffff, 20, M_V8ADDS }
,	{ 0xffffffff, 21, M_V8ADDS }
,	{ 0xffffffff, 22, M_V8ADDS }
,	{ 0xffffffff, 23, M_V8ADDS }
,	{ 0xffffffff, 24, M_V8ADDS }
,	{ 0xffffffff, 25, M_V8ADDS }
,	{ 0xffffffff, 26, M_V8ADDS }
,	{ 0xffffffff, 27, M_V8ADDS }
,	{ 0xffffffff, 28, M_V8ADDS }
,	{ 0xffffffff, 29, M_V8ADDS }
,	{ 0xffffffff, 30, M_V8ADDS }
,	{ 0xffffffff,  1, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  2, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  3, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  4, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  5, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  6, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  7, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  8, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff,  9, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 10, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 11, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 12, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 13, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 14, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 15, A_ITOF,  P_8abcdSI }
,	{ 0xffffffff, 32, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 33, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 34, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 35, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 36, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 37, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 38, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 39, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 40, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 41, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 42, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 43, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 44, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 45, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 46, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 47, A_OR,    P_8abcdSI }
,	{ 0xffffffff, 32, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 33, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 34, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 35, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 36, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 37, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 38, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 39, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 40, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 41, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 42, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 43, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 44, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 45, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 46, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 47, M_V8MIN, P_8abcdSI }
,	{ 0xffffffff, 63, A_OR }  // -1
,	{ 0xffffffff, 48, A_V8ADDS }
,	{ 0xffffffff, 49, A_V8ADDS }
,	{ 0xffffffff, 50, A_V8ADDS }
,	{ 0xffffffff, 51, A_V8ADDS }
,	{ 0xffffffff, 52, A_V8ADDS }
,	{ 0xffffffff, 53, A_V8ADDS }
,	{ 0xffffffff, 54, A_V8ADDS }
,	{ 0xffffffff, 55, A_V8ADDS }
,	{ 0xffffffff, 56, A_V8ADDS }
,	{ 0xffffffff, 57, A_V8ADDS }
,	{ 0xffffffff, 58, A_V8ADDS }
,	{ 0xffffffff, 59, A_V8ADDS }
,	{ 0xffffffff, 60, A_V8ADDS }
,	{ 0xffffffff, 61, A_V8ADDS }
,	{ 0xffffffff, 62, A_V8ADDS }
,	{ 0,           0, A_NOP } // dummy entry for termination
};
