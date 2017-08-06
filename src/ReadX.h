/*
 * ReadC.h
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

#ifndef READC_H_
#define READC_H_

#include "ReaderBase.h"

#include <cstdint>
#include <vector>

using namespace std;


class DebugInfo;

class ReadX : public ReaderBase
{protected:
	const char* Filename;
 protected:
	ReadX(const char* filename);
	void SkipWhitespace();
	void SkipComments();
	[[noreturn]] void InputNotParsable();
};

class ReadX32 : public ReadX
{public:
	ReadX32(const char* filename) : ReadX(filename) {}
	void Read(vector<uint64_t>& instructions, DebugInfo* info = NULL);
};

class ReadX64 : public ReadX
{public:
	ReadX64(const char* filename) : ReadX(filename) {}
	void Read(vector<uint64_t>& instructions, DebugInfo* info = NULL);
};

#endif // READC_H_
