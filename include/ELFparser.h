#ifndef ELF_PARSER
#define ELF_PARSER

#include "ELF32_Parse.h"
#include "ELF64_Parse.h"
#include "filework.h"

typedef struct {
	std::string name;
	std::vector<std::string> callSyms;
	std::vector<std::string> defSyms;
} binFile;

void parse_ELF_File(binFile&);
void parseELF(unsigned char*, binFile&);

#endif