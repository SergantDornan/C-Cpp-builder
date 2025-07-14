#ifndef ELF64_PARSE
#define ELF64_PARSE

#include <cstdint>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

// #define STB_LOCAL       0       // Local symbol
// #define STB_GLOBAL      1       // Global symbol
// #define STB_WEAK        2       // Weak symbol

// #define STT_NOTYPE      0       // Symbol type is unspecified
// #define STT_OBJECT      1       // Symbol is a data object
// #define STT_FUNC        2       // Symbol is a code object
// #define STT_SECTION     3       // Symbol is associated with a section
// #define STT_FILE        4       // Symbol is associated with a file
// #define SHT_REL         9       // Section type: relocation entries without addends
// #define SHT_RELA        10      // Section type: relocation entries with addends
// #define SHT_SYMTAB      2       // Section type: symbol table
// #define SHT_DYNSYM      11      // Section type: dynamic symbol table

// e_ident[0] - должен быть 0x7F
// e_ident[1] - должен быть E
// e_ident[2] - должен быть L
// e_ident[3] - должен быть F
// e_ident[4] - если 1 - 32 битный файл, если 2 - 64 битный
// e_ident[5] - если 1 - LITTLE ENDIAN, если 2 - BIG ENDIAN
// e_ident[6] - версия ELF, обычно 1
// e_ident[7] - идентифицирует OS/ABI
// e_ident[8] - версия ABI
typedef struct {
  unsigned char e_ident[16];      /* Магическое число и другая информация */
  uint16_t e_type;                /* Тип объектного файла */
  uint16_t e_machine;             /* Архитектура */
  uint32_t e_version;             /* Версия объектного файла */
  uint64_t e_entry;               /* Виртуальный адрес точки входа */
  uint64_t e_phoff;               /* Смещение таблицы заголовков программы в файле */
  uint64_t e_shoff;               /* Смещение таблицы заголовков секций в файле */
  uint32_t e_flags;               /* Флаги, зависящие от процессора */
  uint16_t e_ehsize;              /* Размер заголовка ELF в байтах */
  uint16_t e_phentsize;           /* Размер записи таблицы заголовков программы */
  uint16_t e_phnum;               /* Количество записей в таблице заголовков программы */
  uint16_t e_shentsize;           /* Размер записи таблицы заголовков секций */
  uint16_t e_shnum;               /* Количество записей в таблице заголовков секций */
  uint16_t e_shstrndx;            /* Индекс таблицы строк в заголовке секции */
} Elf64_Ehdr;

typedef struct {
 uint32_t p_type;  
 uint32_t p_flags; 
 uint64_t p_offset;
 uint64_t p_vaddr; 
 uint64_t p_paddr; 
 uint64_t p_filesz;
 uint64_t p_memsz; 
 uint64_t p_align; 
} Elf64_Phdr;

typedef struct {
 uint32_t sh_name; 
 uint32_t sh_type;    // Тип секции. 0 - неактивная секция, 1 - ?, 2 - секция таблицы символов (то что нужно),
                      // 3 - секция таблицы строк, 11 - динамические символы...
 uint64_t sh_flags;
 uint64_t sh_addr; 
 uint64_t sh_offset;
 uint64_t sh_size;  
 uint32_t sh_link;  
 uint32_t sh_info;  
 uint64_t sh_addralign;
 uint64_t sh_entsize;  
} Elf64_Shdr; // Секции 


typedef struct {
    uint32_t st_name;       /* Индекс имени символа в таблице строк. */
    unsigned char st_info;  /* Тип и атрибуты символа. */
    unsigned char st_other; /* Зарезервировано. */
    uint16_t st_shndx;      /* Индекс секции, к которой относится символ. Равен 0, если объект не определен в данном файле */
    uint64_t st_value;      /* Значение символа. */
    uint64_t st_size;       /* Размер символа. */
} Elf64_Sym; // Хедер таблицы символов 

// Структура для представления записи перемещения (Relocation entry)
typedef struct {
    uint64_t r_offset;    /* Address */
    uint64_t r_info;      /* Relocation type and symbol index */
    uint64_t r_addend;    /* Addend */ // Используется для RELA (Relocation with Addend)
} Elf64_Rela;

// Relocation (для REL, x86-64)
typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
} Elf64_Rel;


typedef struct {
    std::string name;
    std::vector<std::string> callSyms;
    std::vector<std::string> defSyms;
} Elf64_parse_result; // copy of a binFile structure

void parse64(Elf64_parse_result&, unsigned char*);
void process_symbol_table64(Elf64_parse_result&, unsigned char*, 
    Elf64_Shdr*, Elf64_Shdr*, const bool);

#endif