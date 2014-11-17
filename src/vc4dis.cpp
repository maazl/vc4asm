#include "Disassembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <vector>

using namespace std;

int hexinput = 0;


static void file_load(const char *filename, vector<uint64_t>& memory)
{	FILE *f = fopen(filename, "rb");
	if (!f)
	{	fprintf(stderr, "Failed to read %s: %s\n", filename, strerror(errno));
		return;
	}
	if (fseek(f, 0, SEEK_END) == 0)
	{	long size = ftell(f);
		if (size % sizeof(uint64_t))
			fprintf(stderr, "File size %li of source %s is not a multiple of 64 bit.\n", size, filename);
		size /= sizeof(uint64_t);
		if (size > 0)
			memory.resize(memory.size() + size);
		fseek(f, 0, SEEK_SET);
		memory.resize(memory.size() - size + fread(&memory[memory.size()], size, sizeof(uint64_t), f));
	} else
	{	size_t count = 8192;
		do
		{	memory.resize(memory.size() + count);
			count = fread(&memory[memory.size()], 8192, sizeof(uint64_t), f);
		} while (count == 8192);
		memory.resize(memory.size() - 8192 + count);
	}
	fclose(f);
}

static void file_load_x32(const char *filename, vector<uint64_t>& memory)
{	FILE *f = fopen(filename, "r");
	if (!f)
	{	fprintf(stderr, "Failed to read %s: %s\n", filename, strerror(errno));
		return;
	}
	uint32_t value1, value2;
	while (fscanf(f, "%x,", &value1) == 1)
	{	if (fscanf(f, "%x,", &value2) != 1)
		{	if (feof(f))
			{	fprintf(stderr, "File %s must contain an even number of 32 bit words.\n", filename);
				goto done;
			} else
				break;
		}
		memory.push_back(value1 | (uint64_t)value2 << 32);
	}
	if (!feof(f))
	{	char buf[10];
		*buf = 0;
		fscanf(f, "%9s", buf);
		buf[9] = 0;
		fprintf(stderr, "File %s contains not parsable input %s.\n", filename, buf);
	}
 done:
	fclose(f);
}

static void file_load_x64(const char *filename, vector<uint64_t>& memory)
{	FILE *f = fopen(filename, "r");
	if (!f)
	{	fprintf(stderr, "Failed to read %s: %s", filename, strerror(errno));
		return;
	}
	uint64_t value;
	while (fscanf(f, "%Lx,", &value) == 1)
		memory.push_back(value);
	if (!feof(f))
	{	char buf[10];
		*buf = 0;
		fscanf(f, "%9s", buf);
		buf[9] = 0;
		fprintf(stderr, "File %s contains not parsable input %s.\n", filename, buf);
	}
	fclose(f);
}

void qpu_dis_file(const char *filename) {
	fprintf(stderr, "Disassembling %s\n", filename);
	vector<uint64_t> fragment;
	switch (hexinput)
	{case 32:
		file_load_x32(filename, fragment);
		break;
	 case 64:
		file_load_x64(filename, fragment);
		break;
	 default:
		file_load(filename, fragment);
	}
	if (fragment.size() == 0)
	{	fprintf(stderr, "Couldn't read any data from file %s, aborting.\n", filename);
		return;
	}
	Disassembler dis(stdout);
	dis.Disassemble(fragment);
}

int main(int argc, char * argv[]) {
	if (argc < 2) {
    fprintf(stderr, "vc4dis: Pass in a file name to disassemble as the first argument\n");
    exit(1);
  }
	++argv;
	if (strncmp(*argv, "-x", 2) == 0)
	{	hexinput = atoi(*argv + 2);
		if (!hexinput)
			hexinput = 32;
		++argv;
	}
  while (*argv)
  	qpu_dis_file(*argv++);
}
