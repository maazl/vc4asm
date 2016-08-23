#include "Parser.h"
#include "Validator.h"
#include "Disassembler.h"
#ifdef __linux__
#include "WriteELF.h"
#endif
#include <cstdio>
#include <sstream>
#include <getopt.h>

using namespace std;


#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
/// Byte swap
static inline uint64_t swap_uint64(uint64_t x)
{	x = x << 32 | x >> 32;
	x = (x & 0x0000FFFF0000FFFFULL) << 16 | (x & 0xFFFF0000FFFF0000ULL) >> 16;
	return (x & 0x00FF00FF00FF00FFULL) << 8  | (x & 0xFF00FF00FF00FF00ULL) >> 8;
}
#endif

static const char CPPTemplate[] = ",\n0x%08lx, 0x%08lx";
static const char CPPTemplate_Detailed[] = ",\n/* [0x%08x] */ 0x%08lx, 0x%08lx%s // %-*s | %s";
static const char CPPTemplate_Method[] = ",\n// %s%s";
static const char HTemplate1[] = "#ifndef %1$s_H\n#define %1$s_H\n\nextern unsigned int %1$s[];\n\n";
static const char HTemplate2[] = "#define %s (%s + %u)\n";

bool print_labels(FILE *of, const char *tpl, Parser &parser, unsigned int pos){
	auto &label_map = parser.getLabelsForIntruction(pos, false);
	bool add_newline(false);
	for( auto &label : label_map){
		fprintf(of, CPPTemplate_Method
				+ (tpl-CPPTemplate_Detailed), // offset prevent '\n' in first line
				parser.getLabels()[label.second].Exported?"::":":",
				label.first.c_str());
		add_newline = true;
	}
	return add_newline;
}

void print_hex(FILE *of, Parser &parser){
	const char* tpl = CPPTemplate_Detailed + 2; // no ,\n in the first line

	/* Optimization and macros, etc made it sometimes complex to track which
	 * instruction was created by which chunks of text.
	 * Use disassembly object to generate instruction comment.
	 */
	Disassembler dis;
	stringstream dis_stream;
	string dis_line;

	// Propagete Labels to dis.
	map<size_t,string> l;
	for (auto& label : parser.getLabels())
	{
		l.emplace(label.Value, label.Name);
	}
	dis.ProvideLabels(l);

	for (auto& code : parser.Instructions)
	{	// Search labels
		auto pos = &code - &parser.Instructions[0];
		bool last = (pos == (int)parser.Instructions.size() - 1);
		if (print_labels(of, tpl, parser, 2 * pos))
			tpl = CPPTemplate_Detailed + 1;

		dis.Instructions.clear();
		dis_line.clear();
		dis_stream.str(dis_line); // clear stream

		dis.BaseAddr = sizeof(uint64_t)*pos;
		dis.Instructions.push_back(code);
		dis.Disassemble(dis_stream, true);
		getline(dis_stream, dis_line, '\n');

		fprintf(of, tpl, 2*sizeof(unsigned long)*pos,
				(unsigned long)(code & 0xffffffffULL),
				(unsigned long)(code >> 32),
				last?" ":",",
				26, // min width of following field.
				dis_line.c_str(),
				parser.LineForInstruction[pos].c_str());

		tpl = CPPTemplate_Detailed + 1;
	}

	// Add labels after last instruction, i.e. ':end'.
	print_labels(of, tpl, parser, 2*parser.Instructions.size());
}

int main(int argc, char **argv)
{
	const char* writeBIN = NULL;
	const char* writeCPP = NULL;
	const char* writeCPP2 = NULL;
	const char* writeELF = NULL;
	const char* writeELF2 = NULL;
	const char* writePRE = NULL;
	const char* writeHEADER = NULL;
	bool check = false;
	bool decorated_hex = false;

	Parser parser;

	int c;
	while ((c = getopt(argc, argv, "o:c:e:v:C:H:E:I:V")) != -1)
	{	switch (c)
		{case 'o':
			writeBIN = optarg; break;
		 case 'c':
			writeCPP = optarg; break;
		 case 'C':
			writeCPP2 = optarg; break;
		 case 'H':
			writeHEADER = optarg; break;
#ifdef __linux__
		 case 'e':
			writeELF = optarg; break;
		 case 'E':
			writeELF2 = optarg; break;
#endif
		 case 'I':
			parser.IncludePaths.push_back(optarg); break;
		 case 'V':
			check = true; break;
		 case 'P':
			writePRE = optarg; break;
		 case 'v':
			decorated_hex = true; break;
		}
	}

	if (!writeBIN && !writeCPP && !writeCPP2 && !writePRE && !writeELF && !writeHEADER) {
		fputs("vc4asm V0.2.1\n"
			"Usage: vc4asm [-o <bin-output>] [-{c|C} <c-output>] [-{H} <c-header>] [-v] [-V] <qasm-file(s)>\n"
			" -o<file> Binary output file.\n"
			" -c<file> C output file with trailing ','.\n"
			" -C<file> C output file withOUT trailing ','.\n"
			" -v Add decoration to C output, like labels and instructions.\n"
			" -H<file> C header with list of all ::name labels.\n"
#ifdef __linux__
			" -e<file> Linux ELF output file.\n"
			" -E<file> Linux ELF output file without predefined symbols.\n"
#endif
			" -I<path> Add search path for .include <...>\n"
			" -V       Run instruction verifier and print warnings about suspicious code.\n"
			, stderr);
		return 1;
	}

	if (writePRE)
	{	parser.Preprocessed = fopen(writePRE, "wt");
		if (parser.Preprocessed == NULL)
		{	fprintf(stderr, "Failed to open %s for writing.", writePRE);
			return -1;
		}
	}

	try
	{	while (optind < argc)
		{	parser.ParseFile(argv[optind]);
			++optind;
		}
		parser.EnsurePass2();
		if (!parser.Success)
			throw string("Aborted because of earlier errors.");

		if (check)
		{	Validator v;
			v.Instructions = &parser.Instructions;
			v.Info = &parser;
			v.Validate();
		}

		if (writeHEADER)
		{	FILE* of = fopen(writeHEADER, "wt");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeHEADER);
				return -1;
			}

			// Truncate extension, i.e. '.c'. Warn if header without
			// suffix not match with writeCPP* name.
			string sc(writeCPP2?writeCPP2:(writeCPP?writeCPP:writeHEADER));
			string prog = sc.substr(0, sc.find_last_of("."));

			string sh(writeHEADER);
			if (prog != sh.substr(0, sh.find_last_of(".")))
			{ fprintf(stderr, "Warning: Header file name not match to c file. Name of binary array " \
					"derived from %s-argument.\n %s,%s\n", writeCPP2?"-C":"-c", writeCPP2, writeHEADER);
			};

			fprintf(of, HTemplate1, prog.c_str());

			for (auto& label : parser.getLabels())
			{	if (label.Exported)
				{ fprintf(of, HTemplate2, label.Name.c_str(), prog.c_str(), label.Value/4 );
				}
			}
			fputs("\n#endif\n", of);
			fclose(of);
		}

		if (writeCPP)
		{	FILE* of = fopen(writeCPP, "wt");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeCPP);
				return -1;
			}

			if (decorated_hex)
			{
				print_hex(of, parser);
			}else
			{
				const char* tpl = CPPTemplate + 2; // no ,\n in the first line
				for (auto code : parser.Instructions)
				{	fprintf(of, tpl, (unsigned long)(code & 0xffffffffULL), (unsigned long)(code >> 32) );
					tpl = CPPTemplate;
				}
			}

			fputs(",\n", of);
			fclose(of);
		}
		if (writeCPP2)
		{	FILE* of = fopen(writeCPP2, "wt");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeCPP2);
				return -1;
			}

			if (decorated_hex)
			{
				print_hex(of, parser);
			}else
			{ const char* tpl = CPPTemplate + 2; // no ,\n in the first line
				for (auto code : parser.Instructions)
				{ fprintf(of, tpl, (unsigned long)(code & 0xffffffffULL), (unsigned long)(code >> 32) );
					tpl = CPPTemplate;
				}
			}

			fputc('\n', of);
			fclose(of);
		}

		if (writeBIN)
		{
			/*#if (defined(__BIG_ENDIAN__) && __BIG_ENDIAN__) || (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN)
			for (auto& i : memory)
				i = swap_uint64(i);
			#endif*/
			FILE* of = fopen(writeBIN, "wb");
			if (of == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeBIN);
				return -1;
			}
			fwrite(&*parser.Instructions.begin(), sizeof(uint64_t), parser.Instructions.size(), of);
			fclose(of);
		}

#ifdef __linux__
		if (writeELF)
		{	WriteELF we;
			we.Target = fopen(writeELF, "wb");
			if (we.Target == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeELF);
				return -1;
			}
			we.Write(parser.Instructions, parser, writeELF);
			fclose(we.Target);
		}
		if (writeELF2)
		{	WriteELF we;
			we.Target = fopen(writeELF2, "wb");
			if (we.Target == NULL)
			{	fprintf(stderr, "Failed to open %s for writing.", writeELF2);
				return -1;
			}
			we.NoStandardSymbols = true;
			we.Write(parser.Instructions, parser, writeELF);
			fclose(we.Target);
		}
#endif
	} catch (const string& msg)
	{	fputs(msg.c_str(), stderr);
	  fputc('\n', stderr);
		return 1;
	}

	return !parser.Success;
}
