#ifndef SWAP_ENDIAN
#define SWAP_ENDIAN

#include "ELF64_Parse.h"
#include "ELF32_Parse.h"
#include "lowFuncs.h"

void swapBytesElfHeader(Elf64_Ehdr*);
void swapBytesProgHeader(Elf64_Phdr*);
void swapBytesSectionHeader(Elf64_Shdr*);
void swapBytesSymHeader(Elf64_Sym*);
void swapBytesRela(Elf64_Rela*);
void swapBytesRel(Elf64_Rel*);

void swapBytesElfHeader(Elf32_Ehdr*);
void swapBytesProgHeader(Elf32_Phdr*);
void swapBytesSectionHeader(Elf32_Shdr*);
void swapBytesSymHeader(Elf32_Sym*);


#endif