/*
 * ReadC.cpp
 *
 *  Created on: 06.08.2017
 *      Author: mueller
 */

#include "ReadX.h"
#include "utils.h"


ReadX::ReadX(const char* filename)
: ReaderBase(checkedopen(filename, "r"))
, Filename(filename)
{}

void ReadX::SkipWhitespace()
{	(void)checkedfscanf(Source, "%*[ \t\n]");
}

void ReadX::SkipComments()
{	do
	{	SkipWhitespace();
		char buf[2];
		if (checkedfscanf(Source, "/%1[/*]", buf) != 1)
			return;
		if (*buf == '/')
			(void)checkedfscanf(Source, "%*[^\n]"); // Skip to end of line
		else
		{	int c;
			do // Skip to next */
			{	(void)checkedfscanf(Source, "%*[^*]*");
				c = checkedfgetc(Source);
			} while (c != EOF && c != '/');
		}
	} while (!feof(Source));
}

void ReadX::InputNotParsable()
{	long pos = ftell(Source);
	char buf[20];
	*buf = 0;
	(void)checkedfscanf(Source, "%19s", buf);
	buf[19] = 0;
	throw MSG.HEX_FILE_NOT_PARSABLE.toMsg(Filename, buf, pos);
}


void ReadX32::Read(vector<uint64_t>& instructions, DebugInfo* info)
{	uint32_t value1, value2;
	while ((SkipComments(), checkedfscanf(Source, "%" SCNx32 ",", &value1)) == 1)
	{	SkipComments();
		if (checkedfscanf(Source, "%" SCNx32 ",", &value2) != 1)
		{	if (feof(Source))
				throw MSG.HEX_FILE_ODD_SIZED.toMsg(Filename);
			break;
		}
		instructions.push_back(value1 | (uint64_t)value2 << 32);
	}
	if (!feof(Source))
		InputNotParsable();
}

void ReadX64::Read(vector<uint64_t>& instructions, DebugInfo* info)
{	uint64_t value;
	while ((SkipComments(), fscanf(Source, "%" SCNx64 ",", &value)) == 1)
		instructions.push_back(value);
	if (!feof(Source))
		InputNotParsable();
}
