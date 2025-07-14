#include "ELF32_Parse.h"
#include "ELF64_Parse.h"
#include "lowFuncs.h"
#include "SwapEndian.h"
#include "filework.h"

typedef struct {
	std::string name;
	std::vector<std::string> callSyms;
	std::vector<std::string> defSyms;
} binFile;

binFile parse_ELF_File(const std::string&);