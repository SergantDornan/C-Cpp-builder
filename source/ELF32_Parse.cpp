#include "ELF32_Parse.h"
#include "SwapEndian.h"

void parse32(Elf32_parse_result& result, unsigned char* elf, unsigned long elf_size){
    if(elf_size == 0) elf_size = 0xFFFFFFFF;

    Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)elf;
    const bool correctEndian = (checkSystemEndian() == elfHeader -> e_ident[5]);
    if(!correctEndian) swapBytesElfHeader(elfHeader);
    
    // Проверка без переполнения: сначала отсекаем e_shoff > elf_size,
    // затем сравниваем количество секций с тем, сколько реально помещается.
    if(elfHeader->e_shoff > elf_size ||
        elfHeader->e_shnum > (elf_size - elfHeader->e_shoff) / sizeof(Elf32_Shdr)){
        return;
    }

    if (elfHeader -> e_shnum > 0) {
        Elf32_Shdr* sectionHeaders = (Elf32_Shdr*)(elf + elfHeader -> e_shoff);
        for(int i = 0; i < elfHeader -> e_shnum; ++i){
            Elf32_Shdr* sH = &sectionHeaders[i];
            if(!correctEndian) swapBytesSectionHeader(sH);
        }
        for (size_t i = 0; i < elfHeader -> e_shnum; ++i) {
            Elf32_Shdr* sH = &sectionHeaders[i];
            if (sH -> sh_type == 2 || sH -> sh_type == 11)
                process_symbol_table32(result, elf, sH, sectionHeaders, correctEndian, elf_size, elfHeader->e_shnum);
        }
    } else {
        std::cerr << "=========================== ERROR ===========================" << std::endl;
        std::cerr << "ELF32_Parse.cpp" << std::endl;
        std::cerr << "File: " << result.name << " has no Section Table (wtf?)" << std::endl;
        std::cerr << "This is very weird, try running belder with -reb flag or with \"clear\" option" << std::endl;
        std::cerr << std::endl;
        return;   
    }
}


void process_symbol_table32(Elf32_parse_result& result, unsigned char* elf, Elf32_Shdr* sH,
    Elf32_Shdr* sectionHeaders, const bool correctEndian, unsigned long elf_size, uint16_t shnum)
{
    if(sH->sh_offset > elf_size || sH->sh_size > elf_size - sH->sh_offset){
        return;
    }

    Elf32_Sym* symbol_table = (Elf32_Sym*)(elf + sH->sh_offset);
    size_t num_symbols = sH->sh_size / sizeof(Elf32_Sym);

    // sh_link - индекс секции строк; обязан быть внутри таблицы секций.
    if(sH->sh_link >= shnum){
        return;
    }

    Elf32_Shdr* strtab_shdr = &sectionHeaders[sH->sh_link];

    if(strtab_shdr->sh_offset > elf_size || strtab_shdr->sh_size > elf_size - strtab_shdr->sh_offset){
        return;
    }
    
    char* string_table = (char*)(elf + strtab_shdr->sh_offset);
    
    for (size_t j = 1; j < num_symbols; ++j) {
        Elf32_Sym* symbol = &symbol_table[j];
        if(!correctEndian) swapBytesSymHeader(symbol);
        
        if(symbol->st_name >= strtab_shdr->sh_size){
            continue;
        }
        
        const char* symbol_name = string_table + symbol->st_name;
        unsigned char symbol_info = symbol->st_info;
        unsigned char symbol_type = symbol_info & 0x0F;
        unsigned char symbol_bind = symbol_info >> 4;
        uint16_t symbol_section_index = symbol->st_shndx;
        
        if(symbol_section_index == 0){
            if(symbol_bind == 1 || symbol_bind == 2){
                result.callSyms.push_back(symbol_name);
            }
        }
        else if((symbol_type == 1 || symbol_type == 2) && symbol_bind != 0){
            result.defSyms.push_back(symbol_name);
        }
    }
}