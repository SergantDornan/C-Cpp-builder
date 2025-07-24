#include "ELFparser.h"

void parseELF(unsigned char* elf, binFile& newfile){
    uint8_t arch = uint8_t(*(elf + 4)); // 1 - 32 бит, 2 - 64 бит
    if(arch == 1) { 
        Elf32_parse_result result;
        parse32(result, elf);
        newfile.callSyms = std::move(result.callSyms);
        newfile.defSyms = std::move(result.defSyms);           
    }
    else if(arch == 2){
        Elf64_parse_result result;
        parse64(result, elf);
        newfile.callSyms = std::move(result.callSyms);
        newfile.defSyms = std::move(result.defSyms);
    } 
    else{
        std::cout << "======================== ERROR ========================" << std::endl;
        std::cout << "File: " << newfile.name << " \nHas weird arch field: " << uint16_t(arch) << std::endl;
        std::cout << std::endl;
    }
}


void parse_ELF_File(binFile& newfile){

	std::ifstream file(newfile.name, std::ios::binary);
    unsigned long size = getFileSize(newfile.name);
    unsigned char* elf = new unsigned char[size];
    file.read((char*)elf, size);
    file.close();
    bool isElf = (uint8_t(*elf) == 127 && uint8_t(*(elf+1)) == 'E' && uint8_t(*(elf+2)) == 'L' && uint8_t(*(elf+3)) == 'F');
    if(!isElf){
        std::cout << "==================================== ERROR ====================================" << std::endl;
        std::cout << "belder thinks that file: " << newfile.name << std::endl;
        std::cout << "is an ELF file but it is not" << std::endl;
        std::cout << "try running belder with -reb flag or with \"clear\" option" << std::endl;
        std::cout << "or maybe you specified some strange file in force link section" << std::endl;
        std::cout << std::endl;
        return;
    }
    parseELF(elf, newfile);
    delete[] elf;
}