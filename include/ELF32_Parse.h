#ifndef ELF32_PARSE
#define ELF32_PARSE

#include <cstdint>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

typedef struct
{
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
    uint32_t	p_type;
    uint32_t	p_offset;
    uint32_t	p_vaddr;
    uint32_t	p_paddr;
    uint32_t	p_filesz;
    uint32_t	p_memsz;
    uint32_t	p_flags;
    uint32_t	p_align;
} Elf32_Phdr;

typedef struct
{
    uint32_t   sh_name;
    uint32_t   sh_type;
    uint32_t   sh_flags;
    uint32_t   sh_addr;
    uint32_t   sh_offset;
    uint32_t   sh_size;
    uint32_t   sh_link;
    uint32_t   sh_info;
    uint32_t   sh_addralign;
    uint32_t   sh_entsize;
} Elf32_Shdr;

typedef struct {
	uint32_t    st_name;      /* Symbol name (index into string table) */
	uint32_t    st_value;     /* Value of the symbol */
	uint32_t    st_size;      /* Associated symbol size */
	unsigned char st_info;      /* Symbol type and binding attributes */
	unsigned char st_other;     /* No meaning, other fields have taken over */
	uint16_t    st_shndx;     /* Section index symbol is defined in */
} Elf32_Sym;

typedef struct {
    std::string name;
    std::vector<std::string> callSyms;
    std::vector<std::string> defSyms;
} Elf32_parse_result; // copy of a binFile structure

void parse32(Elf32_parse_result&, unsigned char*);
void process_symbol_table32(Elf32_parse_result&, unsigned char*, 
    Elf32_Shdr*, Elf32_Shdr*, const bool);

#endif