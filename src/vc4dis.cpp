#include "Disassembler.h"
#include "Validator.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/param.h>

using namespace std;

int hexinput = 0;


#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}
#endif

static void file_load_bin(const char *filename, vector<uint64_t>& memory)
{
	ifstream f{filename};
	if (!f.is_open())
	{
		cerr << "Failed to read " << filename << ": "<<strerror(errno) << endl;
		return;
	}
	vector<char> octets{istreambuf_iterator<char>(f), istreambuf_iterator<char>()};
	if (octets.size() % sizeof(uint64_t))
	{
		cerr << "File size " << octets.size() << " of source " << filename << " is not a multiple of 64 bit." << endl;
	}
	auto raw64 = reinterpret_cast<uint64_t*>(octets.data());
	memory.assign(raw64, raw64 + octets.size() / sizeof(uint64_t));

	#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
	for (auto& i : memory)
		i = swap_uint64(i);
	#endif
}

static void file_load_x32(const char *filename, vector<uint64_t>& memory)
{
	FILE *f = fopen(filename, "r");
	if (!f)
	{	fprintf(stderr, "Failed to read %s: %s\n", filename, strerror(errno));
		return;
	}
	uint32_t value1, value2;
	while ((fscanf(f, "%*[ \t]"), fscanf(f, "//%*[^\n]"), fscanf(f, "%x,", &value1)) == 1)
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
	while ((fscanf(f, "%*[ \t]"), fscanf(f, "//%*[^\n]"), fscanf(f, "%Lx,", &value)) == 1)
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

int main(int argc, char * argv[]) {
	if (argc < 2) {
		fputs("vc4dis V0.1.9\n"
			"Usage: vc4dis [-x[32|64]] [-M] [-F] [-v] [-b <addr>] [-o <out_file>] [-V] <code_file(s)>\n"
			" -x    Hexadecimal input, comma separated (rather than binary).\n"
			" -x64  64 bit formatted hexadecimal input.\n"
			" -M    Do not print simple ALU instructions and load immediate as mov.\n"
			" -F    Print floating point constants as hexadecimal.\n"
			" -v    Binary code and offset as comment behind each line.\n"
			" -v2   Write internal instruction field as comment behind every line also.\n"
			" -b<addr> base address (only for output).\n"
			" -o<file> Write output to this file rather than stdout.\n"
			" -V    Run instruction verifier and print warnings about suspicious code.\n"
			, stderr);
		return 1;
	}

	Disassembler dis;
	dis.Out = stdout;
	bool check = false;

	int c;
	while ((c = getopt(argc, argv, "x::MFv::b:o:V")) != -1)
	{	switch (c)
		{case 'x':
			if (!optarg || (hexinput = atoi(optarg)) == 0)
				hexinput = 32;
			break;
		 case 'M':
			dis.UseMOV = false; break;
		 case 'F':
			dis.UseFloat = false; break;
		 case 'v':
			if (optarg && atoi(optarg) >= 2)
				dis.PrintFields = true;
			dis.PrintComment = true;
			break;
		 case 'b':
			dis.BaseAddr = atol(optarg); break;
		 case 'o':
			dis.Out = fopen(optarg, "w");
			if (!dis.Out)
			{	fprintf(stderr, "Failed to write %s: %s\n", optarg, strerror(errno));
				return 1;
			}
			break;
		 case 'V':
			check = true; break;
		}
	}
	argv += optind;

	for (; *argv; ++argv)
	{	fprintf(stderr, "Disassembling %s...\n", *argv);
		switch (hexinput)
		{default:
			fprintf(stderr, "Invalid argument %i to option -x.", hexinput);
			return 2;
		 case 32:
			file_load_x32(*argv, dis.Instructions);
			break;
		 case 64:
			file_load_x64(*argv, dis.Instructions);
			break;
		 case 0:
			file_load_bin(*argv, dis.Instructions);
			break;
		}
		if (dis.Instructions.size() == 0)
		{	fprintf(stderr, "Couldn't read any data from file %s, aborting.\n", *argv);
			continue;
		}
		dis.ScanLabels();
		dis.Disassemble();

		if (check)
			Validator().Validate(dis.Instructions);
	}
}
