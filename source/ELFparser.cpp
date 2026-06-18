#include "ELFparser.h"

void parseELF(unsigned char* elf, binFile& newfile, unsigned long elf_size){
    if(elf_size == 0) elf_size = 0xFFFFFFFF;
    
    if(elf_size < 52){
        std::cerr << "======================== ERROR ========================" << std::endl;
        std::cerr << "File: " << newfile.name << " \nELF file too small" << std::endl;
        std::cerr << std::endl;
        return;
    }
    
    uint8_t arch = uint8_t(*(elf + 4));
    if(arch == 1) { 
        Elf32_parse_result result;
        parse32(result, elf, elf_size);
        newfile.callSyms = std::move(result.callSyms);
        newfile.defSyms = std::move(result.defSyms);           
    }
    else if(arch == 2){
        Elf64_parse_result result;
        parse64(result, elf, elf_size);
        newfile.callSyms = std::move(result.callSyms);
        newfile.defSyms = std::move(result.defSyms);
    } 
    else{
        std::cerr << "======================== ERROR ========================" << std::endl;
        std::cerr << "File: " << newfile.name << " \nHas weird arch field: " << uint16_t(arch) << std::endl;
        std::cerr << std::endl;
    }
}


void parse_ELF_File(binFile& newfile){

    long fileSize = getFileSize(newfile.name);
    // Минимум для ELF-заголовка (32-бит). Так же отсекает случай, когда
    // getFileSize вернул -1 (файл не открылся): отрицательное < 52.
    if(fileSize < 52)
        return;
    unsigned long size = (unsigned long)fileSize;

    std::ifstream file(newfile.name, std::ios::binary);
    if(!file.is_open())
        return;
    std::vector<unsigned char> buffer(size); // RAII: освобождается на любом выходе
    file.read((char*)buffer.data(), size);
    file.close();
    unsigned char* elf = buffer.data();
    bool isElf = (uint8_t(*elf) == 127 && uint8_t(*(elf+1)) == 'E' && uint8_t(*(elf+2)) == 'L' && uint8_t(*(elf+3)) == 'F');
    if(!isElf){
        // std::cerr << "==================================== ERROR ====================================" << std::endl;
        // std::cerr << "belder thinks that file: " << newfile.name << std::endl;
        // std::cerr << "is an ELF file but it is not" << std::endl;
        // std::cerr << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        // std::cerr << "or maybe you specified some strange file in force link section" << std::endl;
        // std::cerr << std::endl;
        return;
    }
    parseELF(elf, newfile, size);
}