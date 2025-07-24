#include "ELF32_Parse.h"
#include "SwapEndian.h"

void parse32(Elf32_parse_result& result, unsigned char* elf){

    Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)elf;
    const bool correctEndian = (checkSystemEndian() == elfHeader -> e_ident[5]);
    if(!correctEndian) swapBytesElfHeader(elfHeader);
    if (elfHeader -> e_shnum > 0) {
        Elf32_Shdr* sectionHeaders = (Elf32_Shdr*)(elf + elfHeader -> e_shoff);
        for(int i = 0; i < elfHeader -> e_shnum; ++i){
            Elf32_Shdr* sH = &sectionHeaders[i];
            if(!correctEndian) swapBytesSectionHeader(sH);
        }
        for (size_t i = 0; i < elfHeader -> e_shnum; ++i) {
            Elf32_Shdr* sH = &sectionHeaders[i];
            if (sH -> sh_type == 2 || sH -> sh_type == 11) //  2 - тип таблицы символов, 11 - динамических символов
                process_symbol_table32(result, elf, sH, sectionHeaders, correctEndian);    
        }
    } else {
        std::cout << "=========================== ERROR ===========================" << std::endl;
        std::cout << "ELF32_Parse.cpp" << std::endl;
        std::cout << "File: " << result.name << " has no Section Table (wtf?)" << std::endl;
        std::cout << "This is very weird, try running belder with -reb flag or with \"clear\" option" << std::endl;
        std::cout << std::endl;
        return;   
    }
}


void process_symbol_table32(Elf32_parse_result& result, unsigned char* elf, Elf32_Shdr* sH, 
    Elf32_Shdr* sectionHeaders, const bool correctEndian)
{
    Elf32_Sym* symbol_table = (Elf32_Sym*)(elf + sH->sh_offset);
    // Определить количество символов в таблице.  Размер таблицы в байтах / размер одного элемента.
    size_t num_symbols = sH->sh_size / sizeof(Elf32_Sym);
    // заголовок секции таблицы строк
    Elf32_Shdr* strtab_shdr = &sectionHeaders[sH->sh_link];
    // указатель на начало таблицы строк
    char* string_table = (char*)(elf + strtab_shdr->sh_offset);
    for (size_t j = 1; j < num_symbols; ++j) {
        Elf32_Sym* symbol = &symbol_table[j];
        if(!correctEndian) swapBytesSymHeader(symbol);
        const char* symbol_name = string_table + symbol->st_name;
        // Получить тип и привязку символа
        unsigned char symbol_info = symbol->st_info;
        unsigned char symbol_type = symbol_info & 0x0F; //ELF64_ST_TYPE(symbol_info) разворачивается в это. Нижние 4 бита.
        unsigned char symbol_bind = symbol_info >> 4;  //ELF64_ST_BIND(symbol_info) разворачивается в это.  Верхние 4 бита.
        // Получить индекс секции, к которой относится символ
        uint16_t symbol_section_index = symbol->st_shndx;
        if((symbol_type == 1 || symbol_type == 2) && symbol_bind != 2 && symbol_bind != 0){
            // symbol_type == 1 || symbol_type == 2 - OBJECT or FUNC - функция или переменная
            // WEAK и LOCAL не включаем потому что не прикольно
            result.defSyms.push_back(symbol_name);
        }

        if(symbol_type == 0 && symbol_section_index == 0 
            && (symbol_bind == 1 || symbol_bind == 2))
        {
            // symbol_type == 0 => type == NOTYPE, то есть не знаем какой тип
            // symbol_section_index == 0 => не определено ни в одной секции
            // symbol_bind == 1 || symbol_bind == 2 - GLOBAL или WEAK
            // => символ вызывается но не определен в файле, его надо найти где то еще
            result.callSyms.push_back(symbol_name);
        }
    }
}