/*
 * ReadBin.cpp
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

#include "ReadBin.h"


#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}
#endif


ReadBin::ReadBin(const char* filename)
: ReaderBase(checkedopen(filename, "rb"))
, Filename(filename)
{}

void ReadBin::Read(vector<uint64_t>& instructions)
{
	if (fseek(Source, 0, SEEK_END) == 0)
	{	long size = ftell(Source);
		if (size % sizeof(uint64_t))
			MSG.BINARY_FILE_ODD_SIZED.toMsg(Filename).print();
		size /= sizeof(uint64_t);
		size_t oldsize = instructions.size();
		if (size > 0)
			instructions.resize(oldsize + size);
		checkedfseek(Source, 0, SEEK_SET);
		instructions.resize(oldsize + checkedfread(&instructions[oldsize], sizeof(uint64_t), size, Source));
	} else
	{	size_t count = 8192;
		size_t oldsize;
		do
		{	oldsize = instructions.size();
			instructions.resize(oldsize + count);
			count = checkedfread(&instructions[oldsize], sizeof(uint64_t), 8192, Source);
		} while (count > 0);
		if (count < 8192)
			instructions.resize(oldsize + count);
		if (!feof(Source))
			MSG.BINARY_FILE_ODD_SIZED.toMsg(Filename).print();
	}
	#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
	for (auto& i : instructions)
		i = swap_uint64(i);
	#endif
}
