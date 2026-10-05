#ifndef ELF_PARSER
#define ELF_PARSER

#include "ELF32_Parse.h"
#include "ELF64_Parse.h"
#include "filework.h"

#define SYM_UNKNOWN 2

typedef struct binFile {
	std::string name;
	std::vector<std::string> callSyms;
	std::vector<std::string> defSyms;
	std::vector<char> callStrong;
	std::vector<char> defStrong;
	uint16_t type;
	uint32_t arch;
	std::vector<binFile> members;
	long offset;
	bool parsed;
} binFile;

void parse_ELF_File(binFile&);
void parseELF(unsigned char*, binFile&, unsigned long elf_size = 0);

#endif