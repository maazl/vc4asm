/*
 * ReadBin.h
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

#ifndef READBIN_H_
#define READBIN_H_

#include "ReaderBase.h"

#include <cstdint>
#include <vector>

using namespace std;


class DebugInfo;

class ReadBin : public ReaderBase
{protected:
	const char* Filename;
 public:
	ReadBin(const char* filename);
	void Read(vector<uint64_t>& instructions);
};

#endif // READBIN_H_
