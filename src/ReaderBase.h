/*
 * ReaderBase.h
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

#ifndef READERBASE_H_
#define READERBASE_H_

#include "utils.h"

#include <cstdio>

using namespace std;


// Work around because gcc can't define a constexpr struct inside a class, part 1.
#define MSG ReaderMSG
#include "Reader.MSG.h"
#undef MSG

/// Common services for reader classes.
class ReaderBase
{public:
	// Work around because gcc can't define a constexpr struct inside a class, part 2.
	//#include "Reader.MSG.h"
	static constexpr const struct ReaderMSG MSG = ReaderMSG;

 protected:
	FILEguard Source;
	ReaderBase(FILE* source) : Source(source) {}
};

#endif // READERBASE_H_
